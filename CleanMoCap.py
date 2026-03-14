import unreal

def clean_mocap_sequence(sequence_path):
    # Load the sequence
    level_sequence = unreal.load_asset(sequence_path)
    
    # Get all tracks (bones)
    bindings = level_sequence.get_bindings()
    
    for binding in bindings:
        tracks = binding.get_tracks()
        for track in tracks:
            # We target Transform sections
            sections = track.get_sections()
            for section in sections:
                # Apply a Euclidean Filter to reduce noise
                # unreal.MovieSceneFilter (Simplified API logic)
                filter_options = unreal.MovieSceneFilterKeysOptions()
                filter_options.tolerance = 0.001
                section.simplify_keys(filter_options)
                
    unreal.log("NexusCap: Keyframe cleaning complete.")

# Usage
clean_mocap_sequence('/Game/Cinematics/Takes/MyRecordedTake')
