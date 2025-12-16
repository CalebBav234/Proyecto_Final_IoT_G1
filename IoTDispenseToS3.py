import json
import boto3
from datetime import datetime

s3 = boto3.client('s3')
BUCKET_NAME = 'pill-dispenser-analytics-us-east-2-7375388'

def lambda_handler(event, context):
    # Extract data from IoT message
    thing_name = event.get('thing_name', 'unknown')
    timestamp_ms = event.get('event_timestamp', int(datetime.now().timestamp() * 1000))
    
    # Convert timestamp to datetime
    dt = datetime.fromtimestamp(timestamp_ms / 1000)
    year = dt.year
    month = dt.month
    day = dt.day
    
    # Build S3 key with proper partitions
    s3_key = f"dispense_completed/year={year}/month={month}/day={day}/{timestamp_ms}.json"
    
    # Write to S3
    try:
        s3.put_object(
            Bucket=BUCKET_NAME,
            Key=s3_key,
            Body=json.dumps(event),
            ContentType='application/json'
        )
        print(f"Successfully wrote to s3://{BUCKET_NAME}/{s3_key}")
        return {'statusCode': 200, 'body': 'Success'}
    except Exception as e:
        print(f"Error: {str(e)}")
        raise e