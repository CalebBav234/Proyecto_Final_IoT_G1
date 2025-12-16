import json
import boto3
from datetime import datetime

s3 = boto3.client('s3')
BUCKET_NAME = 'pill-dispenser-analytics-us-east-2-7375388'

def lambda_handler(event, context):
    thing_name = event.get('thing_name', 'unknown')
    timestamp_ms = event.get('event_timestamp', int(datetime.now().timestamp() * 1000))
    
    # Convert timestamp to date parts
    dt = datetime.fromtimestamp(timestamp_ms / 1000)
    year, month, day = dt.year, dt.month, dt.day
    
    # Prepare data for schedule_monitor table
    schedule_data = {
        'command_id': event.get('last_command_id'),
        'timestamp': timestamp_ms,
        'thing_name': thing_name,
        'pill_name': event.get('pill_name', 'Unknown'),
        'pill_hour': event.get('pill_hour', 0),
        'pill_minute': event.get('pill_minute', 0),
        'user_id': event.get('user_id', 'default_user'),  # Add if available
        'event_type': 'scheduled',
        'reported': {
            'buzzer_enabled': event.get('buzzer_enabled', False),
            'last_dispense': event.get('last_dispense', 0),
            'reported_command_id': event.get('last_command_id', 0)
        },
        'year': year,
        'month': month,
        'day': day
    }
    
    # Build S3 key
    s3_key = f"schedule_monitor/year={year}/month={month}/day={day}/{timestamp_ms}.json"
    
    # Write to S3
    try:
        s3.put_object(
            Bucket=BUCKET_NAME,
            Key=s3_key,
            Body=json.dumps(schedule_data),
            ContentType='application/json'
        )
        print(f"Wrote to s3://{BUCKET_NAME}/{s3_key}")
        return {'statusCode': 200}
    except Exception as e:
        print(f"Error: {str(e)}")
        raise e