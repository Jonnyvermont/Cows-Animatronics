import requests
import time

# Function to play an animation
def play_animation(animation_index):
    url = 'http://localhost:59224/PlaybackState/'
    
    # Set the playback state to play the selected animation
    data = {
        'selectedAnimationIndex': animation_index,
        'isPlaying': True
    }
    
    # Make the PUT request to set the playback state
    response = requests.put(url, json=data)
    
    if response.status_code == 200:
        print(f"Playing animation {animation_index}")
    else:
        print(f"Failed to play animation {animation_index}")

# Function to get playback state details
def get_playback_state():
    url = 'http://localhost:59224/PlaybackState/'
    
    # Make the GET request to fetch the playback state
    response = requests.get(url)
    
    if response.status_code == 200:
        return response.json()
    else:
        print("Failed to get playback state")
        return None

# Function to play the animations in sequence with the special animation in between
def play_animations_with_intermission(animations, intermission_index):
    original_count = len(animations)
    current_animation = 0

    while True:
        # Play the current animation
        play_animation(animations[current_animation])
        
        # Wait for the current animation to finish
        while True:
            playback_state = get_playback_state()
            if playback_state and not playback_state['isPlaying']:
                break
            time.sleep(1)  # Check every second
        
        # Play the intermission animation
        play_animation(intermission_index)

        # Wait for the intermission animation to finish
        while True:
            playback_state = get_playback_state()
            if playback_state and not playback_state['isPlaying']:
                break
            time.sleep(1)  # Check every second
        
        # Move to the next animation
        current_animation = (current_animation + 1) % original_count

# Fetch the list of animations
response = requests.get('http://localhost:59224/Animations/')
if response.status_code == 200:
    animations = response.json()
    print("Available animations:", animations)

    # Identify the index of the "time to show" animation
    intermission_animation = "time to show"
    intermission_index = animations.index(intermission_animation) if intermission_animation in animations else -1

    if intermission_index == -1:
        print(f"Intermission animation '{intermission_animation}' not found in the list of animations.")
    else:
        # Play the animations with intermission
        play_animations_with_intermission(animations, intermission_index)
else:
    print("Failed to fetch animations")
