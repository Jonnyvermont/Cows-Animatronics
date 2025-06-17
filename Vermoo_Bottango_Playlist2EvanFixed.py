import requests
import time

# Initialize variables
animIndex = 0  # Track the current animation we want to play
maxAnim = 5    # Number of animations available
timeToWait = 10  # Time to wait between animations in seconds

# Function to play an animation
def playAnim(animation_index):
    url = 'http://localhost:59224/PlaybackState/'
    print(f"Sending request to play animation at index {animation_index}")
    
    # Set the playback state to play the selected animation
    data = {
        'playbackTimeInMS': 0,  # Reset playback time to zero to start from the beginning
        'selectedAnimationIndex': animation_index,
        'isPlaying': True
    }
    print(f"Request payload: {data}")
    
    # Make the PUT request to set the playback state
    try:
        response = requests.put(url, json=data)
        print(f"Response: {response.status_code}, {response.text}")
        return response.status_code in (200, 202)
    except requests.exceptions.RequestException as e:
        print(f"Request failed: {e}")
        return False

# Function to check if the animation is done
def animIsPlaying():
    url = 'http://localhost:59224/PlaybackState/'
    print("Sending request to get playback state")
    
    # Make the GET request to fetch the playback state
    try:
        response = requests.get(url)
        print(f"Response: {response.status_code}")
        if response.status_code == 200:
            playback_state = response.json()
            print(f"playback state is : {playback_state['isPlaying']}")
            return playback_state['isPlaying']
        else:
            return False
    except requests.exceptions.RequestException as e:
        print(f"Request failed: {e}")
        return False

# Main loop to play animations in sequence
while True:
    # Play the current animation
    print(f"request playing {animIndex}")
    if playAnim(animIndex):

        print("waiting 5 second to allow jogs, etc...")
        time.sleep(5)  # Wait a second so we're not spamming checks

        # Increment the animation index
        animIndex += 1
        
        # Wait until the current animation is done
        while animIsPlaying():
            print("waiting...")
            time.sleep(1)  # Wait a second so we're not spamming checks
        

        print("Done!")
        # If we are at the last animation, reset to the first
        if animIndex >= maxAnim:
            animIndex = 0            
            print("Loop Back to 0")
        
        # Wait before playing the next animation

        print("waiting to trigger next")
        time.sleep(timeToWait)
        print("wait complete")
    else:
        print("Failed to play animation. Exiting loop.")
        break  # Exit the loop if unable to play the animation
