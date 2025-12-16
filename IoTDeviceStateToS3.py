import json
import boto3
from datetime import datetime

s3 = boto3.client('s3')
BUCKET_NAME = 'pill-dispenser-analytics-us-east-2-7375388'

def lambda_handler(event, context):
    thing_name = event.get('thing_name', 'unknown')
    timestamp_ms = event.get('event_timestamp', int(datetime.now().timestamp() * 1000))
    reported_state = event.get('reported_state', {})
    
    # Convert timestamp to date parts
    dt = datetime.fromtimestamp(timestamp_ms / 1000)
    year, month, day = dt.year, dt.month, dt.day
    
    # Prepare data for device_state table (with map structure)
    device_state_data = {
        'thing_name': thing_name,
        'event_timestamp': timestamp_ms,
        'reported_state': {k: str(v) for k, v in reported_state.items()},  # Convert all to strings for map
        'year': year,
        'month': month,
        'day': day
    }
    
    # Build S3 key
    s3_key = f"device_state/year={year}/month={month}/day={day}/{timestamp_ms}.json"
    
    # Write to S3
    try:
        s3.put_object(
            Bucket=BUCKET_NAME,
            Key=s3_key,
            Body=json.dumps(device_state_data),
            ContentType='application/json'
        )
        print(f"Wrote to s3://{BUCKET_NAME}/{s3_key}")
        return {'statusCode': 200}
    except Exception as e:
        print(f"Error: {str(e)}")
        raise e