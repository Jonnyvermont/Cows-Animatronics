import requests
import time
from datetime import datetime

# Initialize variables
animIndex = 0  # Track the current animation we want to play
maxAnim = 7    # Number of animations available
timeToWait = 3  # Time to wait between animations in seconds

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
def animNotDone():
    url = 'http://localhost:59224/PlaybackState/'
    print("Sending request to get playback state")
    
    # Make the GET request to fetch the playback state
    try:
        response = requests.get(url)
        print(f"Response: {response.status_code}, {response.text}")
        if response.status_code >= 200 and response.status_code < 300:
            playback_state = response.json()
            isPlaying = playback_state['isPlaying']
            playbackTimeInMS = playback_state['playbackTimeInMS']
            durationInMS = playback_state['durationInMS']
            elapsed_time = playbackTimeInMS / 1000.0
            duration = durationInMS / 1000.0
            print(f"Animation playing. Elapsed time: {elapsed_time:.2f}s / {duration:.2f}s")
            return isPlaying
        else:
            return False
    except requests.exceptions.RequestException as e:
        print(f"Request failed: {e}")
        return False

# Ask user if they want to use operating hours
use_operating_hours = input("Use Operating Hours? (Y/N): ").strip().lower() == 'y'

# Fetch the list of animations
animations = fetchAnimations()
maxAnim = len(animations)  # Set maxAnim to the number of available animations

if maxAnim == 0:
    print("No animations available. Exiting.")
    exit(1)

# Main loop to play animations in sequence
while True:
    if use_operating_hours:
        if is_within_operating_hours():
            # Play the current animation
            if playAnim(animIndex):
                # Increment the animation index
                animIndex += 1
                
                # Wait until the current animation is done
                while animNotDone():
                    time.sleep(1)  # Wait a second so we're not spamming checks
                
                # If we are at the last animation, reset to the first
                if animIndex >= maxAnim:
                    animIndex = 0
                
                # Wait before playing the next animation
                time.sleep(timeToWait)
            else:
                print("Failed to play animation. Exiting loop.")
                break  # Exit the loop if unable to play the animation
        else:
            print("Outside operating hours. Waiting to start...")
            time.sleep(60)  # Check every minute if we are within operating hours
    else:
        # Play the current animation
        if playAnim(animIndex):
            # Increment the animation index
            animIndex += 1
            
            # Wait until the current animation is done
            while animNotDone():
                time.sleep(1)  # Wait a second so we're not spamming checks
            
            # If we are at the last animation, reset to the first
            if animIndex >= maxAnim:
                animIndex = 0
            
            # Wait before playing the next animation
            time.sleep(timeToWait)
        else:
            print("Failed to play animation. Exiting loop.")
            break  # Exit the loop if unable to play the animation
