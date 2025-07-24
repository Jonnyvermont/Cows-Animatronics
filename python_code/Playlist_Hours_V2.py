import requests # makes requests to Bottange API
import time
from datetime import datetime

# Initialize variables
animIndex = 0  # Track the current animation we want to play
maxAnim = 5    # Number of animations available
timeToWait = 10  # Time to wait between animations in seconds

# Define operating hours (24-hour format)
operating_hours = {
    0: ("16:00", "20:30"),  # Monday
    1: ("16:00", "20:30"),  # Tuesday
    2: ("16:00", "20:30"),  # Wednesday
    3: ("16:00", "20:30"),  # Thursday
    4: ("13:00", "21:00"),  # Friday
    5: ("13:00", "20:30"),  # Saturday
    6: ("13:00", "20:30")   # Sunday
}

# Function to check if the current time is within operating hours
def is_within_operating_hours():
    now = datetime.now()
    current_day = now.weekday()
    current_time = now.time()

    if current_day in operating_hours:
        start_time_str, end_time_str = operating_hours[current_day]
        start_time = datetime.strptime(start_time_str, "%H:%M").time()
        end_time = datetime.strptime(end_time_str, "%H:%M").time()

        if start_time <= current_time <= end_time:
            return True

    return False

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

# Ask user if they want to use operating hours
use_operating_hours = input("Use Operating Hours? (Y/N): ").strip().lower() == 'y'

# Main loop to play animations in sequence
while True:
    if use_operating_hours:
        if is_within_operating_hours():
            # Play the current animation
            print(f"request playing {animIndex}")
            if playAnim(animIndex):
                print("waiting 5 seconds to allow jogs, etc...")
                time.sleep(5)  # Wait 5 seconds to allow for jogs
                
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
        else:
            print("Outside operating hours. Waiting to start...")
            time.sleep(60)  # Check every minute if we are within operating hours
    else:
        # Play the current animation
        print(f"request playing {animIndex}")
        if playAnim(animIndex):
            print("waiting 5 seconds to allow jogs, etc...")
            time.sleep(5)  # Wait 5 seconds to allow for jogs
            
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
