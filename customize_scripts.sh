#!/bin/bash

# CHANGE TO YOUR BUCKET NAME
BUCKET_NAME="bloom-kinesis-bucket-1"
aws s3api create-bucket --bucket ${BUCKET_NAME} --region us-east-1

ACCOUNT_ID="796900460656"
find . -type f -name "*.json" -exec sed -i "s/768910357496/${ACCOUNT_ID}/g" {} +



