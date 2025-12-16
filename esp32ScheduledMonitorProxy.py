import json
import boto3
from datetime import datetime, timezone, timedelta
from typing import Dict, Any

# ---------- CONFIG ----------
MAIN_LAMBDA_NAME = "esp32ColorLambda"
MAIN_LAMBDA_REGION = "us-east-1"

S3_BUCKET = "pill-dispenser-analytics-us-east-2-7375388"
S3_REGION = "us-east-2"

BOLIVIA_TZ = timezone(timedelta(hours=-4))

# ---------- AWS CLIENTS ----------
lambda_client = boto3.client("lambda", region_name=MAIN_LAMBDA_REGION)
s3_client = boto3.client("s3", region_name=S3_REGION)


# ---------- HELPERS ----------
def bolivia_timestamp() -> int:
    """Return current timestamp in Bolivia (UTC-4)"""
    return int(datetime.now(timezone.utc).astimezone(BOLIVIA_TZ).timestamp())


def detect_event_type(event: Dict[str, Any]) -> str:
    """
    Lightweight classification ONLY for analytics partitioning.
    Do NOT modify event semantics.
    """
    if event.get("dispense_status") or event.get("dispensed_color"):
        return "dispense_completed"

    if event.get("pill_hour") is not None and event.get("pill_minute") is not None:
        return "schedule_monitor"

    return "device_state"


def build_s3_key(event_type: str) -> str:
    now = datetime.now(timezone.utc)
    return (
        f"{event_type}/"
        f"year={now.year}/month={now.month:02d}/day={now.day:02d}/"
        f"event_{int(now.timestamp() * 1000)}.json"
    )


def store_event_in_s3(event: Dict[str, Any]) -> None:
    event_type = detect_event_type(event)
    s3_key = build_s3_key(event_type)

    s3_client.put_object(
        Bucket=S3_BUCKET,
        Key=s3_key,
        Body=json.dumps(event),
        ContentType="application/json"
    )


def forward_to_main_lambda(event: Dict[str, Any]) -> Dict[str, Any]:
    response = lambda_client.invoke(
        FunctionName=MAIN_LAMBDA_NAME,
        InvocationType="RequestResponse",
        Payload=json.dumps(event)
    )

    payload = response["Payload"].read()
    return json.loads(payload) if payload else {}


# ---------- HANDLER ----------
def lambda_handler(event, context):
    print("Proxy received event:", json.dumps(event))

    try:
        # Add proxy metadata (non-invasive)
        event["_proxy_received_bz"] = bolivia_timestamp()

        # Store raw event for analytics (S3 / Athena / QuickSight)
        store_event_in_s3(event)

        # Forward event to main Lambda (business logic)
        response_payload = forward_to_main_lambda(event)

        print("Main Lambda response:", json.dumps(response_payload))
        return response_payload

    except Exception as e:
        print("Proxy error:", str(e))
        import traceback
        traceback.print_exc()

        return {
            "statusCode": 500,
            "error": str(e)
        }

