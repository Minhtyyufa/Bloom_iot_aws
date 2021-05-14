# Installing the software

Please see [BUILD.MD](./BUILD.MD) first

Start by editing [customize_scripts.sh](./customize_scripts.sh). Change the bucket name to the bucket name that you desire. Then change the ACCOUNT_ID variable to be your aws account number.

Run [customize_scripts.sh](./customize_scripts.sh) like so:

`
chmod 755 customize_scripts.sh
./customize_scripts.sh
`

If the bucket name already exists, change the bucket name and try again.

Once you've found the bucket name, go into the [setup.sh](./setup.sh) script and change the bucket name to match your bucket name. Then run the script like so:

`
chmod 755 setup.sh
./setup.sh
`
Take note of the printed values at the end.

## Setting up the Registration website
To set up the first create an empty GitHub repository and place the files located in the [bloom-iot-device-signup folder](./bloom-iot-device-signup) into the empty repository. Then go to the **AWS Amplify console** and click **Host web app**. Click GitHub, login to your GitHub account, and authorize AWS Amplify to access your GitHub account. Then select the repository containing the files in the [bloom-iot-device-signup folder](./bloom-iot-device-signup). Hit Continue until AWS Amplify starts deploying your web app. 

### Adding the callback to the user pool
Go to the **Cognito Console** and go to **User Pools**. Click the **bloom-pool**. Under the **App Integration** section, choose **App client settings**. Make sure the box under **Enabled Identity Providers** that says **Cognito User Pool** is checked. For **Callback URL(s)** and **Sign out URL(s)** enter the URL where the website will be hosted. If being run locally enter, **http://localhost:8000**. If not, enter the URI of your Amplify hosted website.  Under **Allowed OAuth Flows**, select **Authorization code grant** and **implicit grant**. Under **Allowed OAuth Scopes**, select **email** and **openid**. Choose **Save changes**.

Click **Domain name** under  **App integration** and enter a **domain name**. Choose **Check availability**. Choose **Save changes**. 

### What to replace in the index.html file

**`__REGION__`** → The region such as **us-east-1**. It can be found in the cognito webdomain\
**`__CLIENT_ID__`** → **Cognito** → **User Pools** → **bloom-pool** → **App Integration** → **App client settings** → **bloom-app-client**→ **ID**
**`__APP_WEB_DOMAIN__`** → the cognito webdomain you entered before without "https://". For example, bloom.auth.us-east-1.amazoncognito.com\
**`__AMPLIFY_URI__`** → the URI of website hosted by Amplify \
**`__USER_POOL_ID__`** → **Cognito** → **User Pools** → **bloom-pool** → **General Settings** → **Pool ID**\
**`__IDENTITY_POOL_ID__`** → Go to **Federated Identities** → **bloom_identity_pool** → **Edit Identity Pool** → **Identity pool ID**\

## Connect ESP8266 to IoT
In the **Arduino IDE** go to **File menu** → **preferences** and paste in the additional boards manager URL's: http://arduino.esp8266.com/stable/package_esp8266com_index.json \
Then go to **Tools** → **Board** → **Board Manager** and search for and install **esp8266** by **ESP8266 Community** \
Next go to **Tools** → **Board** → **The arduino board** (the one used is **NodeMCU 1.0 (ESP-12E Module)**)

###Libraries to install
Go to **Sketch** → **Include Library** → **Manage Libraries** \
Install:
**Adafruit_SGP30**, **PubSubClient**, **DHT sensor library**, and **ESP8266WiFi**

###What to replace/inpup in the code
**ssid** → **Wifi SSID** \
**password** → **Wifi password** \
**certficatePemCrt** → **Certificate Pem** (This is created in the website when a device is registered).\
**privatePemKey** → **Certificate Private** (This is also created in the website when a device is registerd).\
**AWS_IOT_ENDPOINT** → **IoT core** → **Settings** → **Endpoint**
#### Formating the certificates
Do either format:\
Format 1: Add `R"EOF(` to the beginning of the certificate and `)EOF";` at the end of the certificate.\
Format 2: Surround each line of the certificate with a " and end each line with \\. For example "certiciateline" \\


