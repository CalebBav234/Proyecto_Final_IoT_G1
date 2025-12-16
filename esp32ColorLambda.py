# esp32ColorLambda.py - Optimized version
import json
import os
import time
import traceback
import re
from datetime import datetime, timezone, timedelta
from decimal import Decimal

import boto3
from boto3.dynamodb.conditions import Key, Attr
from dateutil import parser

# ----- Configuration via environment variables -----
USER_TABLE = os.environ.get('USER_TABLE', 'UserThings')
EVENTS_TABLE = os.environ.get('EVENTS_TABLE', 'ColorControllerEvents')
IOT_REGION = os.environ.get('IOT_REGION', 'us-east-2')

# ----- AWS clients -----
dynamo = boto3.resource('dynamodb', region_name=os.environ.get('DDB_REGION', 'us-east-1'))
user_table = dynamo.Table(USER_TABLE)
events_table = dynamo.Table(EVENTS_TABLE)

iot = boto3.client('iot-data', region_name=IOT_REGION)

# ----- Timezone: Bolivia UTC-4 -----
BOLIVIA_TZ = timezone(timedelta(hours=-4))

# ----- Color map & helpers -----
DISPENSE_ANGLES = {
    "WHITE": 0, "CREAM": 30, "BROWN": 60,
    "RED": 90, "BLUE": 120, "GREEN": 150, "OTHER": 180
}
VALID_COLORS = set(DISPENSE_ANGLES.keys())


# ---------------- Utility functions ----------------
def now_bz_epoch_seconds():
    """Return current time as epoch seconds in Bolivia timezone."""
    return int(datetime.now(BOLIVIA_TZ).timestamp())


def iso_from_epoch_bz(ts):
    """Return ISO string in Bolivia tz for readable logs."""
    return datetime.fromtimestamp(int(ts), tz=BOLIVIA_TZ).isoformat()


def log_exception(prefix="Exception"):
    print(prefix)
    traceback.print_exc()


def format_time_12h(hour, minute):
    """Convert 24-hour time to 12-hour format with AM/PM."""
    period = "AM" if hour < 12 else "PM"
    display_hour = hour % 12
    if display_hour == 0:
        display_hour = 12
    return f"{display_hour}:{minute:02d} {period}"


def convert_decimals(obj):
    """Convert DynamoDB Decimal types to Python native types for JSON serialization."""
    if isinstance(obj, list):
        return [convert_decimals(i) for i in obj]
    elif isinstance(obj, dict):
        return {k: convert_decimals(v) for k, v in obj.items()}
    elif isinstance(obj, Decimal):
        return int(obj) if obj % 1 == 0 else float(obj)
    return obj


# ---------------- Time parsing ----------------
def parse_alexa_time(time_str):
    """
    Robust Alexa time parser: returns (hour, minute) in 24-hour format or raises ValueError
    Handles:
      - "08:00", "T08:00", "2025-01-01T08:00" (24-hour format)
      - "8 AM", "8 a.m.", "8am", "8pm", "8 p.m." (12-hour with AM/PM)
      - "8", "8:30" (plain numbers - defaults to AM if < 12)
      - "eight o'clock", "noon", "midnight"
      - "24:00" → normalized to 0:00 (midnight)
    """
    if not time_str or not isinstance(time_str, str):
        raise ValueError("empty time_str")

    s = time_str.strip()
    s_lower = s.lower()

    # Handle special cases
    if 'noon' in s_lower:
        return 12, 0
    if 'midnight' in s_lower:
        return 0, 0

    # 1) ISO-like forms: "YYYY-MM-DDTHH:MM", "T08:00", "08:00", "20:00"
    iso_match = re.search(r'(\d{1,2}):(\d{2})', s)
    if iso_match:
        hour = int(iso_match.group(1))
        minute = int(iso_match.group(2))
        
        if hour == 24:
            hour = 0
        
        print(f"[parse_alexa_time] Parsed ISO time: {hour}:{minute:02d} from '{time_str}'")
        return hour % 24, minute

    # 2) Plain hour with optional minute and AM/PM
    norm = re.sub(r'\.', '', s_lower)
    m = re.match(r'^\s*(\d{1,2})(?::(\d{2}))?\s*(am|pm)?\s*$', norm)
    if m:
        hour = int(m.group(1))
        minute = int(m.group(2)) if m.group(2) else 0
        ampm = m.group(3)
        
        if hour == 24:
            hour = 0
            
        if ampm:
            ampm = ampm.lower()
            if ampm == 'pm' and hour != 12:
                hour += 12
            elif ampm == 'am' and hour == 12:
                hour = 0
            print(f"[parse_alexa_time] Parsed with AM/PM: {hour}:{minute:02d} from '{time_str}'")
            return hour % 24, minute
        else:
            print(f"[parse_alexa_time] Parsed plain number: {hour}:{minute:02d} from '{time_str}'")
            return hour % 24, minute

    # 3) Last resort: try dateutil parser
    try:
        dt = parser.parse(s)
        print(f"[parse_alexa_time] Parsed via dateutil: {dt.hour}:{dt.minute:02d} from '{time_str}'")
        return dt.hour, dt.minute
    except Exception as e:
        raise ValueError(f"unrecognized time format: {time_str}") from e


# ---------------- Alexa response helpers ----------------
def build_response(text, end_session=False):
    return {
        "version": "1.0",
        "sessionAttributes": {},
        "response": {
            "outputSpeech": {"type": "PlainText", "text": text},
            "shouldEndSession": end_session
        }
    }


def build_response_with_session(text, session_attrs=None, end_session=False):
    return {
        "version": "1.0",
        "sessionAttributes": session_attrs or {},
        "response": {
            "outputSpeech": {"type": "PlainText", "text": text},
            "shouldEndSession": end_session
        }
    }


# ---------------- Dynamo/Device helper functions ----------------
def get_user_device(user_id):
    """Query user_table for the device mapped to this user."""
    try:
        resp = user_table.query(KeyConditionExpression=Key('user_id').eq(user_id))
        items = resp.get('Items', [])
        if not items:
            return None
        return items[0]
    except Exception as e:
        print("get_user_device error:", e)
        log_exception()
        return None


# ---------------- IoT Rule Event Handlers ----------------
def handle_dispense_completed(event):
    """
    Handle completed dispense events from IoT Rule (esp32_dispense_data_collection).
    This is triggered when device reports dispense completion in shadow.
    """
    try:
        print("[handle_dispense_completed] incoming:", json.dumps(event, default=str))
        
        thing_name = event.get('thing_name')
        command_id = event.get('command_id')
        dispensed_color = event.get('dispensed_color')
        dispensed_angle = event.get('dispensed_angle')
        dispense_status = event.get('dispense_status')
        last_dispense = event.get('last_dispense')
        
        # Color sensor data
        dominant_color = event.get('dominant_color')
        r = event.get('r')
        g = event.get('g')
        b = event.get('b')

        now_bz = datetime.now(BOLIVIA_TZ)
        timestamp_bz = int(now_bz.timestamp())

        # Find the original dispense_request to get pill_name and user_id
        pill_name = 'UNKNOWN'
        user_id = 'SYSTEM'
        
        if command_id:
            try:
                resp = events_table.query(
                    KeyConditionExpression=Key('command_id').eq(int(command_id))
                )
                items = resp.get('Items', [])
                if items:
                    original_request = items[0]
                    pill_name = original_request.get('pill_name', 'UNKNOWN')
                    user_id = original_request.get('user_id', 'SYSTEM')
                    print(f"[handle_dispense_completed] Found original request: pill={pill_name}, user={user_id}")
            except Exception as e:
                print(f"[handle_dispense_completed] Error finding original request: {e}")
        
        # If we still don't have pill_name/user_id, try to get from most recent schedule
        if pill_name == 'UNKNOWN' and dispensed_color:
            try:
                resp = events_table.scan(
                    FilterExpression=Attr('event_type').eq('schedule_update') & 
                                   Attr('thing_name').eq(thing_name) &
                                   Attr('color').eq(dispensed_color),
                    Limit=1
                )
                items = resp.get('Items', [])
                if items:
                    pill_name = items[0].get('pill_name', 'UNKNOWN')
                    user_id = items[0].get('user_id', 'SYSTEM')
                    print(f"[handle_dispense_completed] Inferred from schedule: pill={pill_name}, user={user_id}")
            except Exception as e:
                print(f"[handle_dispense_completed] Error inferring from schedule: {e}")

        # Parse last_dispense safely
        last_dispense_epoch = 0
        if isinstance(last_dispense, str) and last_dispense:
            try:
                # Handle ISO format like "2025-12-09T13:55:00Z"
                dt = datetime.fromisoformat(last_dispense.replace("Z", "+00:00"))
                last_dispense_epoch = int(dt.timestamp())
            except Exception:
                last_dispense_epoch = 0
        elif isinstance(last_dispense, (int, float)):
            last_dispense_epoch = int(last_dispense)

        # Store dispense completion event
        item = {
            'command_id': int(command_id) if command_id else int(time.time() * 1000),
            'timestamp': timestamp_bz,
            'thing_name': thing_name,
            'pill_name': pill_name,
            'color': dispensed_color or 'UNKNOWN',
            'user_id': user_id,
            'event_type': 'dispense_completed',
            'reported': {
                'dispensed_color': dispensed_color or 'UNKNOWN',
                'dispensed_angle': int(dispensed_angle) if dispensed_angle is not None else 0,
                'dispense_status': dispense_status or 'unknown',
                'last_dispense': last_dispense_epoch,
                'dominant_color': dominant_color or 'Unknown',
                'r': int(r) if r is not None else 0,
                'g': int(g) if g is not None else 0,
                'b': int(b) if b is not None else 0
            }
        }
        
        print("[handle_dispense_completed] Writing item to DynamoDB:", json.dumps(item, default=str))
        events_table.put_item(Item=item)
        
        return {'statusCode': 200, 'body': json.dumps('Dispense completion logged')}
        
    except Exception as e:
        print("Error in handle_dispense_completed:", str(e))
        log_exception()
        return {'statusCode': 500, 'body': json.dumps(str(e))}


def handle_schedule_monitor(event):
    """
    Handle scheduled time monitor events from IoT Rule (esp32_scheduled_time_monitor).
    This logs when device reports its current schedule configuration.
    """
    try:
        print("[handle_schedule_monitor] incoming:", json.dumps(event, default=str))
        
        thing_name = event.get('thing_name')
        pill_name = event.get('pill_name', 'UNKNOWN')
        pill_hour = event.get('pill_hour')
        pill_minute = event.get('pill_minute')
        buzzer_enabled = event.get('buzzer_enabled', False)
        last_dispense = event.get('last_dispense', 0)
        last_command_id = event.get('last_command_id', 0)

        now_bz = datetime.now(BOLIVIA_TZ)
        timestamp_bz = int(now_bz.timestamp())

        item = {
            'command_id': int(time.time() * 1000),
            'timestamp': timestamp_bz,
            'thing_name': thing_name,
            'pill_name': pill_name,
            'pill_hour': int(pill_hour) if pill_hour is not None else -1,
            'pill_minute': int(pill_minute) if pill_minute is not None else -1,
            'user_id': 'SYSTEM',
            'event_type': 'scheduled_time_monitor',
            'reported': {
                'buzzer_enabled': buzzer_enabled,
                'last_dispense': int(last_dispense) if last_dispense else 0,
                'reported_command_id': int(last_command_id) if last_command_id else 0
            }
        }
        
        print("[handle_schedule_monitor] Writing item to DynamoDB:", json.dumps(item, default=str))
        events_table.put_item(Item=item)
        
        return {'statusCode': 200, 'body': json.dumps('Schedule monitor logged')}
        
    except Exception as e:
        print("Error in handle_schedule_monitor:", str(e))
        log_exception()
        return {'statusCode': 500, 'body': json.dumps(str(e))}


# ---------------- Core Command Handlers ----------------
def handle_dispense(user_id, thing_name, pill_name):
    """
    Dispense a pill immediately via MQTT command topic.
    Uses MQTT for immediate commands, NOT shadow desired.
    """
    try:
        print(f"[handle_dispense] Looking for pill '{pill_name}' for user {user_id}")
        
        # Find the scheduled color for this pill
        resp = events_table.scan(
            FilterExpression=Attr('user_id').eq(user_id) & 
                           Attr('event_type').eq('schedule_update') &
                           Attr('pill_name').eq(pill_name)
        )
        
        items = resp.get('Items', [])
        print(f"[handle_dispense] Found {len(items)} schedule_update items for {pill_name}")
        
        if not items:
            return build_response(
                f"Pill {pill_name} not found in schedules. Please schedule it first.", 
                end_session=False
            )

        # Use the most recent schedule
        latest = max(items, key=lambda x: int(x.get('timestamp', 0)))
        pill_color = latest.get('color', 'UNKNOWN')
        
        print(f"[handle_dispense] Found color: {pill_color}")

        # Generate command
        command_id = int(time.time() * 1000)
        command_payload = {
            "action": "dispense",
            "pill_name": pill_name,
            "color": pill_color,
            "command_id": command_id
        }
        
        # Publish to command topic (immediate action, not shadow)
        topic = f"esp32/commands/{thing_name}"
        print(f"[handle_dispense] Publishing to topic {topic}: {command_payload}")
        
        iot.publish(
            topic=topic,
            qos=1,  # QoS 1 for at-least-once delivery
            payload=json.dumps(command_payload)
        )

        # Log dispense request in DynamoDB
        now_bz = datetime.now(BOLIVIA_TZ)
        events_table.put_item(Item={
            'command_id': command_id,
            'timestamp': int(now_bz.timestamp()),
            'thing_name': thing_name,
            'pill_name': pill_name,
            'color': pill_color,
            'user_id': user_id,
            'event_type': 'dispense_request',
            'reported': {}
        })
        
        print(f"[handle_dispense] Successfully logged dispense request")

        return build_response(
            f"Dispensing {pill_color.lower()} {pill_name} now. What else can I help you with?", 
            end_session=False
        )

    except Exception as e:
        print(f"[handle_dispense] Error: {e}")
        log_exception()
        return build_response(
            "There was an error requesting the dispense. Try again later.", 
            end_session=False
        )


# ---------------- Alexa Intent Handlers ----------------
def handle_alexa_event(event, context):
    """Main Alexa event handler."""
    try:
        user_id = event['session']['user']['userId']
        print("[handle_alexa_event] user_id:", user_id)

        device = get_user_device(user_id)
        if not device:
            return build_response(
                "No smart pill dispensers are configured for your account.", 
                end_session=True
            )

        thing_name = device['thing_name']
        friendly_name = device.get('description', 'pill dispenser')

        req = event['request']
        req_type = req.get('type')

        if req_type == "LaunchRequest":
            return build_response(
                f"Welcome to {friendly_name}. You can schedule pills, dispense one now, or ask for your next or last pill.", 
                end_session=False
            )

        if req_type == "IntentRequest":
            intent = req.get('intent', {})
            intent_name = intent.get('name')

            # --- SetPillScheduleIntent: Start multi-turn conversation ---
            if intent_name == "SetPillScheduleIntent":
                pill_name_slot = intent.get('slots', {}).get('PillName', {})
                pill_name = pill_name_slot.get('value') if pill_name_slot else None
                
                if not pill_name:
                    return build_response(
                        "I didn't catch the pill name. Please try again.", 
                        end_session=False
                    )
                
                session_attrs = {"pill_name": pill_name}
                return build_response_with_session(
                    f"You said {pill_name}. What color is the pill and what time should I schedule it?",
                    session_attrs=session_attrs,
                    end_session=False
                )

            # --- SetPillTimeIntent: Complete schedule and update shadow desired ---
            elif intent_name == "SetPillTimeIntent":
                session_attrs = event.get('session', {}).get('attributes', {}) or {}
                pill_name = session_attrs.get('pill_name')
                
                color_slot = intent.get('slots', {}).get('Color', {})
                time_slot = intent.get('slots', {}).get('Time', {})

                if not pill_name:
                    return build_response(
                        "I lost track of which pill we were scheduling. Please start over.", 
                        end_session=False
                    )

                if not color_slot.get('value'):
                    return build_response_with_session(
                        "What color is the pill?", 
                        {"pill_name": pill_name}, 
                        end_session=False
                    )
                    
                if not time_slot.get('value'):
                    return build_response_with_session(
                        "At what time should I schedule it?", 
                        {"pill_name": pill_name}, 
                        end_session=False
                    )

                color = color_slot['value'].upper()
                if color not in VALID_COLORS:
                    return build_response_with_session(
                        f"{color} is not valid. Valid colors: {', '.join(sorted(list(VALID_COLORS)))}.", 
                        {"pill_name": pill_name},
                        end_session=False
                    )

                # Parse time
                try:
                    time_str = time_slot['value']
                    print(f"[SetPillTimeIntent] raw Time slot value: {repr(time_str)}")
                    hour, minute = parse_alexa_time(time_str)
                    print(f"[SetPillTimeIntent] parsed hour={hour}, minute={minute}")
                except ValueError as exc:
                    print(f"[SetPillTimeIntent] time parse error: {exc}")
                    return build_response_with_session(
                        "I couldn't understand that time. Please say like 8 AM or 2:30 PM.", 
                        {"pill_name": pill_name},
                        end_session=False
                    )

                command_id = int(time.time() * 1000)

                # Update shadow desired state for OTA configuration
                shadow_payload = {
                    "state": {
                        "desired": {
                            "pill_name": pill_name,
                            "color": color,
                            "pill_hour": hour,
                            "pill_minute": minute,
                            "buzzer_enabled": True,
                            "command_id": command_id
                        }
                    }
                }
                
                try:
                    print(f"[SetPillTimeIntent] Updating shadow for {thing_name}")
                    iot.update_thing_shadow(
                        thingName=thing_name, 
                        payload=json.dumps(shadow_payload)
                    )
                except Exception as e:
                    print("update_thing_shadow error:", e)
                    log_exception()
                    return build_response(
                        "Failed to persist configuration to the device. Try again later.", 
                        end_session=False
                    )

                # Log schedule update
                now_bz = datetime.now(BOLIVIA_TZ)
                events_table.put_item(Item={
                    'command_id': command_id,
                    'timestamp': int(now_bz.timestamp()),
                    'thing_name': thing_name,
                    'pill_name': pill_name,
                    'pill_hour': hour,
                    'pill_minute': minute,
                    'color': color,
                    'buzzer_enabled': True,
                    'user_id': user_id,
                    'event_type': 'schedule_update',
                    'reported': {}
                })

                time_12h = format_time_12h(hour, minute)
                return build_response(
                    f"Scheduled {color.lower()} pill {pill_name} at {time_12h}. What else can I help you with?", 
                    end_session=False
                )

            # --- DispensePillIntent: Immediate dispense via MQTT ---
            elif intent_name == "DispensePillIntent":
                pill_name_slot = intent.get('slots', {}).get('PillName', {})
                pill_name = pill_name_slot.get('value') if pill_name_slot else None
                
                if not pill_name:
                    return build_response(
                        "I didn't catch the pill name. Which pill should I dispense?", 
                        end_session=False
                    )
                
                return handle_dispense(user_id, thing_name, pill_name)

            # --- Query intents ---
            elif intent_name == "GetCurrentPillIntent":
                return get_next_pill(user_id)

            elif intent_name == "GetLastDispensedPillIntent":
                return get_last_dispensed(user_id)

            # --- Built-in intents ---
            elif intent_name == "AMAZON.HelpIntent":
                return build_response(
                    "You can schedule pills, dispense them, or ask about next or last pill.", 
                    end_session=False
                )
                
            elif intent_name in ["AMAZON.StopIntent", "AMAZON.CancelIntent", "AMAZON.NavigateHomeIntent"]:
                return build_response("Goodbye!", end_session=True)
                
            elif intent_name == "AMAZON.FallbackIntent":
                return build_response(
                    "I didn't understand that. You can schedule pills, dispense, or ask about next or last pill.", 
                    end_session=False
                )

        return build_response(
            "I didn't understand that. What would you like to do?", 
            end_session=False
        )

    except Exception as e:
        print("Alexa handler error:", e)
        log_exception()
        return build_response(
            "There was an error processing your request.", 
            end_session=False
        )


# ---------------- Query Functions ----------------
def get_next_pill(user_id):
    """Get the next scheduled pill for the user."""
    try:
        resp = events_table.scan(
            FilterExpression=Attr('user_id').eq(user_id) & 
                           Attr('event_type').eq('schedule_update')
        )
        items = resp.get('Items', [])
        
        if not items:
            return build_response(
                "No pills scheduled. Would you like to schedule one?", 
                end_session=False
            )

        now_bz = datetime.now(BOLIVIA_TZ)
        current_minutes = now_bz.hour * 60 + now_bz.minute
        
        next_pill = None
        min_diff = float('inf')

        for item in items:
            h = int(item.get('pill_hour', 0))
            m = int(item.get('pill_minute', 0))
            pill_minutes = h * 60 + m
            
            if pill_minutes >= current_minutes:
                diff = pill_minutes - current_minutes
            else:
                diff = (24 * 60 - current_minutes) + pill_minutes

            if diff < min_diff:
                min_diff = diff
                next_pill = item

        if next_pill:
            hour = int(next_pill['pill_hour'])
            minute = int(next_pill['pill_minute'])
            time_12h = format_time_12h(hour, minute)
            color = next_pill.get('color', 'UNKNOWN').lower()
            
            return build_response(
                f"Your next scheduled pill is {color} {next_pill['pill_name']} at {time_12h}.", 
                end_session=False
            )
            
        return build_response("No upcoming pills found.", end_session=False)
        
    except Exception as e:
        print("get_next_pill error:", e)
        log_exception()
        return build_response(
            "There was an error fetching your next pill.", 
            end_session=False
        )


def get_last_dispensed(user_id):
    """Get the last dispensed pill for the user."""
    try:
        # Query for dispense_completed events (most accurate)
        resp = events_table.scan(
            FilterExpression=Attr('user_id').eq(user_id) & 
                           Attr('event_type').eq('dispense_completed')
        )
        
        items = resp.get('Items', [])
        
        # Fallback to dispense_request if no completed dispenses
        if not items:
            resp = events_table.scan(
                FilterExpression=Attr('user_id').eq(user_id) & 
                               Attr('event_type').eq('dispense_request')
            )
            items = resp.get('Items', [])
        
        if items:
            # Sort by timestamp descending
            items_sorted = sorted(
                items, 
                key=lambda x: int(x.get('timestamp', 0)), 
                reverse=True
            )
            last = items_sorted[0]
            
            dt = datetime.fromtimestamp(int(last['timestamp']), tz=BOLIVIA_TZ)
            time_12h = format_time_12h(dt.hour, dt.minute)
            color = last.get('color', 'UNKNOWN').lower()
            
            return build_response(
                f"The last dispensed pill was {color} {last['pill_name']} at {time_12h}.", 
                end_session=False
            )
            
        return build_response(
            "No pills have been dispensed yet.", 
            end_session=False
        )
        
    except Exception as e:
        print("get_last_dispensed error:", e)
        log_exception()
        return build_response(
            "There was an error fetching the last dispensed pill.", 
            end_session=False
        )


# ---------------- Main Lambda Handler ----------------
def lambda_handler(event, context):
    """
    Main entry point for Lambda.
    Handles:
    1. IoT Rule events (dispense completion, schedule monitoring)
    2. Alexa Skill requests
    """
    print("=" * 80)
    print("Received event:", json.dumps(event, default=str))
    print("=" * 80)
    
    try:
        # Alexa event (check first as it's most specific)
        if 'session' in event and 'request' in event:
            print("[lambda_handler] Routing to handle_alexa_event")
            return handle_alexa_event(event, context)
        
        # IoT Rule events - check if it's from IoT (has thing_name and event_timestamp)
        if 'thing_name' in event and 'event_timestamp' in event:
            print("[lambda_handler] Detected IoT Rule event")
            
            # Determine which type of IoT event based on available fields
            # IoT Rule: Dispense completed event (has dispensed_color or dispense_status)
            if 'dispensed_color' in event or 'dispense_status' in event:
                print("[lambda_handler] Routing to handle_dispense_completed")
                return handle_dispense_completed(event)
            
            # IoT Rule: Schedule monitor event (has pill_hour and pill_minute)
            elif 'pill_hour' in event and 'pill_minute' in event:
                print("[lambda_handler] Routing to handle_schedule_monitor")
                return handle_schedule_monitor(event)
            
            # Generic IoT event with just thing_name and timestamp
            else:
                print("[lambda_handler] Generic IoT event - minimal data")
                print(f"[lambda_handler] Available keys: {list(event.keys())}")
                # This might be an incomplete event from IoT Rule
                # Log it but don't error
                return {
                    'statusCode': 200,
                    'body': json.dumps('IoT event received but no specific handler matched')
                }

        # Unknown event
        print("[lambda_handler] Unknown event type")
        print(f"[lambda_handler] Event keys: {list(event.keys())}")
        return {
            'statusCode': 400, 
            'body': json.dumps('Unknown event type')
        }
        
    except Exception as e:
        print("Top-level lambda error:", e)
        log_exception()
        return {
            'statusCode': 500, 
            'body': json.dumps(f'Internal error: {str(e)}')
        }