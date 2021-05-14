#!/bin/bash

APPLICATION_NAME="Bloom_IoT_Data"
DELIVERY_STREAM_ARN=$(aws firehose describe-delivery-stream --delivery-stream-name IoT-Destination-Data-Stream | jq .DeliveryStreamDescription.DeliveryStreamARN)

echo "${DELIVERY_STREAM_ARN}"

ROLE_ARN=$(aws kinesisanalytics describe-application --application-name ${APPLICATION_NAME} | jq .ApplicationDetail.InputDescriptions[0].KinesisFirehoseInputDescription.RoleARN)

echo "${ROLE_ARN}"

APP_VERSION_ID=$(aws kinesisanalytics describe-application --application-name ${APPLICATION_NAME} | jq .ApplicationDetail.ApplicationVersionId)

echo "${APP_VERSION_ID}"


aws kinesisanalytics add-application-output --application-name ${APPLICATION_NAME} --current-application-version-id ${APP_VERSION_ID} --application-output "Name=DESTINATION_SQL_BASIC_STREAM,KinesisFirehoseOutput={ResourceARN=${DELIVERY_STREAM_ARN},RoleARN=${ROLE_ARN}},DestinationSchema={RecordFormatType=CSV}"

sleep 5
echo "Adding Aggregate Pipeline"

APP_VERSION_ID=$(aws kinesisanalytics describe-application --application-name ${APPLICATION_NAME} | jq .ApplicationDetail.ApplicationVersionId)

AGGREGATE_DELIVERY_STREAM_ARN=$(aws firehose describe-delivery-stream --delivery-stream-name IoT-Destination-Aggregate-Stream | jq .DeliveryStreamDescription.DeliveryStreamARN)

echo "${AGGREGATE_DELIVERY_STREAM_ARN}"

aws kinesisanalytics add-application-output --application-name ${APPLICATION_NAME} --current-application-version-id ${APP_VERSION_ID} --application-output "Name=DESTINATION_SQL_AGGREGATE_STREAM,KinesisFirehoseOutput={ResourceARN=${DELIVERY_STREAM_ARN},RoleARN=${ROLE_ARN}},DestinationSchema={RecordFormatType=CSV}"




