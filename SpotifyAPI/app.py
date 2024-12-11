from flask import Flask, request, redirect, jsonify
import requests
import base64

app = Flask(__name__)

# Spotify client credentials and redirect URI
client_id = 'b6670992f2904efa886bfbb15f1567d5'
client_secret = '0d4dac3fa2684d70b6d970e8a9cdb811'
redirect_uri = 'http://127.0.0.1:8888/callback'  # Updated redirect URI

@app.route('/callback', methods=['GET'])
def callback():
    code = request.args.get('code')
    state = request.args.get('state')
    
    if state is None:
        # Redirect with an error if the state is missing
        return redirect('/#' + 'error=state_mismatch')

    # Prepare the token request
    auth_string = f"{client_id}:{client_secret}"
    auth_header = base64.b64encode(auth_string.encode()).decode('utf-8')

    auth_options = {
        'url': 'https://accounts.spotify.com/api/token',
        'data': {
            'code': code,
            'redirect_uri': redirect_uri,
            'grant_type': 'authorization_code'
        },
        'headers': {
            'Content-Type': 'application/x-www-form-urlencoded',
            'Authorization': f'Basic {auth_header}'
        }
    }

    # Make the request for the token
    response = requests.post(auth_options['url'], data=auth_options['data'], headers=auth_options['headers'])
    if response.status_code == 200:
        return jsonify(response.json())
    else:
        return jsonify({'error': 'Failed to retrieve access token'}), response.status_code

if __name__ == '__main__':
    app.run(port=8888, debug=True)  # Updated to listen on port 8888
