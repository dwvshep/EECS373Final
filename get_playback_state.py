import requests

url = "https://api.spotify.com/v1/me/player"
headers = {
    "Authorization": "Bearer BQBphBlxf5qidztTyjJDUPAR1BEu5MGYwPnQy2mYIKILTxApXUAWwQoCJ9N708USU8N7xC3Zi_hZXX27DRWAgkuafKBvFOxPDE6uP1vhgrmWxcIY7srUfW7aSOPKar4eXNHCbF_Hq9NOhGsXyseXxVaaki5tTrKrVlg1iCjs_kmU9N2nff6YQkh_6OVSTnEebEtxw0G9Z6NUcWfV406wgqI1K53q0ITH4Oo"
}
# params = {
#     "market": "US"  # Specify the market as 
# }

response = requests.get(url, headers=headers)

if response.status_code == 200:
    player_data = response.json()
    print("Player data:", player_data)
elif response.status_code == 404:
    print("Playback not active or player not found. Start playback on a Spotify client and try again.")
else:
    print("Failed to retrieve player data:", response.status_code, response.text)
