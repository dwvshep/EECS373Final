import requests
import os
import base64

def get_refresh_token():
    # Set your refresh token, client ID, and client secret here
    refresh_token = 'AQBrGMr6CnL21Nr2fIZNMRW4RnRSxl3OKoHNb70h3ocdJa0LtNylDyfCtdFCzzGVHRA7XEmlMMmLGka1pUXGJNX1dgJfmJ98b2MJBwvtr9ppVn2CM4lqDoa6vIBSqgY5Vjg'
    client_id = 'b6670992f2904efa886bfbb15f1567d5'
    client_secret = '0d4dac3fa2684d70b6d970e8a9cdb811'  # Replace with your actual client secret

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
        print("Access Token:", response_data['access_token'])
    else:
        print("Error:", response_data)

    # Optionally update refresh token if a new one is provided in the response
    if 'refresh_token' in response_data:
        os.environ['SPOTIFY_REFRESH_TOKEN'] = response_data['refresh_token']
    
    return response_data

# Example usage
if __name__ == "__main__":
    token_data = get_refresh_token()
    print("Response Data:", token_data)
