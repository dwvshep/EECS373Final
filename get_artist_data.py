import requests

url = "https://api.spotify.com/v1/artists/4Z8W4fKeB5YxbusRsdQVPb"
headers = {
    "Authorization": "Bearer BQDbFod0jlBiMlwvr9rPYB0C0tbmm24qu-GXiCLgt0ltJLEXHUNzeETec_ZZr2sOGhgjCIeklF47g9jlsKR6cKlOOnJs_j2PFwKrRPY1T4eSTd4S04SDnz5eOLEUqE6YpnVSb_1Fklzcyst4XiUrMK0Gts7SYzEnfMztjesLeVZEAGkHnnQCzDNL0srDWYw-SEGjl8infw_tQmgPae8AftjossBKEmMFMyo"
}

response = requests.get(url, headers=headers)

if response.status_code == 200:
    artist_data = response.json()
    print("Artist data:", artist_data)
else:
    print("Failed to retrieve artist data:", response.status_code, response.text)
