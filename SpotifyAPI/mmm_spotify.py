import requests
import os
import base64
import time 
import serial

ser = serial.Serial('/dev/tty.usbserial-A50285BI', 9600, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE)

def get_refresh_token():

    refresh_token = 'AQBrGMr6CnL21Nr2fIZNMRW4RnRSxl3OKoHNb70h3ocdJa0LtNylDyfCtdFCzzGVHRA7XEmlMMmLGka1pUXGJNX1dgJfmJ98b2MJBwvtr9ppVn2CM4lqDoa6vIBSqgY5Vjg'
    client_id = 'b6670992f2904efa886bfbb15f1567d5'
    client_secret = '0d4dac3fa2684d70b6d970e8a9cdb811'

    # Encode client_id and client_secret as Base64 for the Authorization header
    auth_header = base64.b64encode(f"{client_id}:{client_secret}".encode()).decode()

    url = "https://accounts.spotify.com/api/token"
    
    payload = {
        'grant_type': 'refresh_token',
        'refresh_token': refresh_token
    }
    
    headers = {
        'Authorization': f"Basic {auth_header}",
        'Content-Type': 'application/x-www-form-urlencoded'
    }
    
    response = requests.post(url, data=payload, headers=headers)
    response_data = response.json()
    
    # Check if the response contains an access token
    if 'access_token' in response_data:
        os.environ['SPOTIFY_ACCESS_TOKEN'] = response_data['access_token']
    else:
        print("Error:", response_data)

    # update refresh token if a new one is provided in the response
    if 'refresh_token' in response_data:
        os.environ['SPOTIFY_REFRESH_TOKEN'] = response_data['refresh_token']
    
    return response_data

def get_tempo(access_token, track_id):
        
    url = f"https://api.spotify.com/v1/audio-features/{track_id}"
    headers = {
        "Authorization": f"Bearer {access_token}"
    }
    
    response = requests.get(url, headers=headers)
    
    if response.status_code == 200:
        audio_data = response.json()
        return audio_data['tempo']
    else:
        return {"error": f"Request failed with status code {response.status_code}: {response.text}"}


def set_shuffle(state, access_token):
    
    device_id = "919a220ae0c240f9035f268fc80f4c6e4ed9ae16"
    url = f"https://api.spotify.com/v1/me/player/shuffle"
    headers = {
        "Authorization": f"Bearer {access_token}"
    }
    params = {
        "state": str(state).lower(),
        "device_id": device_id
    }

    requests.put(url, headers=headers, params=params)



def get_playback(access_token):

    url = "https://api.spotify.com/v1/me/player/currently-playing"
    auth = f"Bearer {access_token}"

    headers = {
        "Authorization": auth 
    }

    response = requests.get(url, headers=headers)

    # Check if the request was successful
    if response.status_code == 200:
        return response.json()
    else:
        print(f"Error: {response.status_code}")
        print(response.text)

def get_artist_info(access_token, artist_id):
    url = f"https://api.spotify.com/v1/artists/{artist_id}"
    headers = {
        "Authorization": f"Bearer {access_token}"
    }
    
    response = requests.get(url, headers=headers)
    
    if response.status_code == 200:
        return response.json()
    else:
        response.raise_for_status()

def shuffle_playlist(access_token, genre):   
    playlist_ids = {}
    playlist_ids['jazz'] = "spotify:playlist:3yg6SIrPxrE01wSQSxwJPm"
    playlist_ids['rock'] = "spotify:playlist:5oeN1VPmwxGv43CQfFU8Iv"
    playlist_ids['groovy'] = "spotify:playlist:40rV1MFCzHiDABLnUrD3a3"
    playlist_ids['christmas'] = "spotify:playlist:7ijeXQbzrGevZVaYXbFaHy"

    device_id = "919a220ae0c240f9035f268fc80f4c6e4ed9ae16"

    url = f"https://api.spotify.com/v1/me/player/play?device_id={device_id}"
    auth = f"Bearer {access_token}"

    headers = {
        "Authorization": auth,
        "Content-Type": "application/json"
    }

    data = {
        "context_uri": playlist_ids[genre],
        "offset": {
            "position": 0
        },
        "position_ms": 0
    }

    response = requests.put(url, headers=headers, json=data)

    # Check if the request was successful
    if response.status_code in (200, 204):
        print("Playback started successfully.")
    else:
        print(f"Error: {response.status_code}")
        print(response.text)


def pause(access_token):
    url = "https://api.spotify.com/v1/me/player/pause"
    auth = f"Bearer {access_token}"

    headers = {
        "Authorization": auth
    }

    response = requests.put(url, headers=headers)

    # Check if the request was successful
    if response.status_code in (200, 204):  
        print("Playback paused successfully.")
    else:
        print(f"Error: {response.status_code}")
        print(response.text)
    

def play(access_token):
    url = "https://api.spotify.com/v1/me/player/play"
    auth = f"Bearer {access_token}"

    headers = {
        "Authorization": auth
    }

    response = requests.put(url, headers=headers)

    # Check if the request was successful
    if response.status_code in (200, 204):
        print("Playback Resumed successfully.")
    else:
        print(f"Error: {response.status_code}")
        print(response.text)
        

def next_song(access_token):
    url = "https://api.spotify.com/v1/me/player/next"
    auth = f"Bearer {access_token}"

    headers = {
        "Authorization": auth
    }

    response = requests.post(url, headers=headers)

    # Check if the request was successful
    if response.status_code in (200, 204): 
        print("Skipped successfully.")
    else:
        print(f"Error: {response.status_code}")
        print(response.text)

def send_song_data_to_stm(playback_data, artist_data):
    print("attempting to send to stm")
    name = playback_data['item']['name']
    name_length = len(name)

    artists = [artist['name'] for artist in playback_data['item']['artists']]
    all_artists = ', '.join(artists)
    artists_length = len(all_artists)

    progress = str(playback_data['progress_ms'])
    duration = str(playback_data['item']['duration_ms'])

    genres = artist_data['genres']


    # Send song data to STM32
    ser.write(f"{name_length}#".encode())
    ser.write(f"{name}@".encode())
    ser.write(f"{artists_length}^".encode())
    ser.write(f"{all_artists}*".encode())
    ser.write(f"{progress}!".encode())
    ser.write(duration.encode())
    ser.write("$".encode())  # Indicate end of song playback

def receive_data_from_stm():
    if ser.in_waiting > 0:  # Check for incoming data
        incoming_data = ser.readline().decode().strip()
        print(f"Received from STM32: {incoming_data}")
        return incoming_data
    return None


if __name__ == "__main__":
    # ################# MAIN CODE #####################
    try:
        token_data = get_refresh_token()
        
        while True:
            # Continuously listen for responses from STM32
            response = receive_data_from_stm()
            if response:
                if response == "ACK":
                    print("STM32 acknowledged song data.")
                
                elif response == "PLAY":
                    print("Play song")
                    play(token_data['access_token'])
                
                
                elif response == "PAUSE":
                    print("Pause song")
                    pause(token_data['access_token'])

                elif response == "NEXT":
                    print("STM32 requested next song.")
                    next_song(token_data['access_token'])
                    time.sleep(1)
                    playback_data = get_playback(token_data['access_token'])
                    artist_data = get_artist_info(token_data['access_token'], playback_data['item']['artists'][0]['id'])
                    send_song_data_to_stm(playback_data, artist_data)
                
                elif response == "ROCK":
                    print("rock playing!")
                    set_shuffle(True, token_data['access_token'])
                    shuffle_playlist(token_data['access_token'], 'rock')
                    next_song(token_data['access_token'])

                    time.sleep(1)
                    playback_data = get_playback(token_data['access_token'])
                    artist_data = get_artist_info(token_data['access_token'], playback_data['item']['artists'][0]['id'])

                    send_song_data_to_stm(playback_data, artist_data)
                
                elif response == "JAZZ":
                    print("jazz playing!")
                    set_shuffle(True, token_data['access_token'])
                    shuffle_playlist(token_data['access_token'], 'jazz')
                    next_song(token_data['access_token'])

                    time.sleep(1)
                    playback_data = get_playback(token_data['access_token'])
                    artist_data = get_artist_info(token_data['access_token'], playback_data['item']['artists'][0]['id'])

                    send_song_data_to_stm(playback_data, artist_data)
                

                elif response == "GROV":
                    print("=groovy playing!")
                    set_shuffle(True, token_data['access_token'])
                    shuffle_playlist(token_data['access_token'], 'groovy')
                    next_song(token_data['access_token'])

                    time.sleep(1)
                    playback_data = get_playback(token_data['access_token'])
                    artist_data = get_artist_info(token_data['access_token'], playback_data['item']['artists'][0]['id'])

                    send_song_data_to_stm(playback_data, artist_data)
                
                elif response == "CHRI":
                    print("christmas playing!")
                    set_shuffle(True, token_data['access_token'])
                    shuffle_playlist(token_data['access_token'], 'christmas')
                    next_song(token_data['access_token'])

                    time.sleep(1)
                    playback_data = get_playback(token_data['access_token'])
                    artist_data = get_artist_info(token_data['access_token'], playback_data['item']['artists'][0]['id'])

                    send_song_data_to_stm(playback_data, artist_data)

                else:
                    print(f"Unknown command from STM32: {response}")

    except KeyboardInterrupt:
        ser.close()
        print("Serial communication closed.")

    ######################## TESTING FUNCTIONS ##################
    # token_data = get_refresh_token()
    # playback_data = get_playback(token_data['access_token'])
    # set_shuffle(True, token_data['access_token'])
    # shuffle_playlist(token_data['access_token'], 'christmas')
    # next_song(token_data['access_token'])

  
    # print(playback_data)
    # tempo = get_tempo(token_data['access_token'], playback_data['item']['id'])
    # print(tempo)
    # artist_data = get_artist_info(token_data['access_token'], playback_data['item']['artists'][0]['id'])

    # send_song_data_to_stm(playback_data, artist_data)

    #shuffle_playlist(token_data['access_token'], 'groovy')
    #pause(token_data['access_token'])
    #play(token_data['access_token'])
    #next_song(token_data['access_token'])
