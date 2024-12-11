import requests

url = "https://accounts.spotify.com/api/token"
headers = {
    "Content-Type": "application/x-www-form-urlencoded"
}
data = {
    "grant_type": "authorization_code",
    "code": "BQD_6SSBhG5D5m5bp5Fr6gi00OLFoOk9F4gCXa_wp1F-97uQcKpVTp5mEYwfa3glMW8XsgxTBtyCwciNYkrYqfGpgG-UFoFju9MgSdaSd_WYipjS2T0",  # Replace with the fresh code from Step 1
    "client_id": "b6670992f2904efa886bfbb15f1567d5",
    "client_secret": "0d4dac3fa2684d70b6d970e8a9cdb811"
}

response = requests.post(url, headers=headers, data=data)

if response.status_code == 200:
    print("Access token:", response.json().get("access_token"))
else:
    print("Failed to get token:", response.status_code, response.text)
