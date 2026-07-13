"""
Module: Offline Oracle Evaluator
Reads the static dataset and the C++ benchmark results, translates the 
board states into the Go Text Protocol (GTP), and queries the MoHex engine 
to get a continuous win-probability score for each chosen move.
"""

import pandas as pd
import subprocess
import os
import sys
import re

# Padded board constants from C++
N = 9
PADDED_N = N + 2

ORACLE_BUDGET = 1.0

def idx_to_gtp(idx):
    """
    Converts a 1D C++ padded array index to a GTP coordinate (e.g., 'a1', 'k11').
    GTP typically skips the letter 'I'.
    """
    row = idx // PADDED_N
    col = idx % PADDED_N
    
    # Letters: A-L (skipping I)
    # letters = "abcdefghjkl"
    letters = "abcdefghjklmnopqrstuvwxyz"
    
    if 1 <= col <= N and 1 <= row <= N:
        return f"{letters[col - 1]}{row}"
    return "pass"

def evaluate_move(state_row, chosen_move_idx):
    """
    Spawns a single MoHex instance, plays the move, reads the evaluation, 
    and closes safely without deadlocking.
    """
    cmds = ["clear_board", f"boardsize {N}"]
    
    # 1. Setup the board from the dataset
    # Turn 1 = First (Black), Turn 2 = Second (White)
    for i in range(1, len(state_row)):
        cell_val = int(state_row[i])
        if cell_val == 1:
            cmds.append(f"play black {idx_to_gtp(i - 1)}")
        elif cell_val == 2:
            cmds.append(f"play white {idx_to_gtp(i - 1)}")
            
    # 2. Play the move chosen by our C++ agent
    current_turn = "black" if int(state_row[0]) == 1 else "white"
    cmds.append(f"play {current_turn} {idx_to_gtp(chosen_move_idx)}")
    
    # 3. Ask MoHex to evaluate the resulting board via a short search
    # We ask MoHex to search for 0.5 seconds to get a solid evaluation
    cmds.append(f"param_mohex max_time {ORACLE_BUDGET}")
    
    # Generate a move for the OPPONENT to force MoHex to print evaluation stats
    opp_turn = "white" if current_turn == "black" else "black"
    cmds.append(f"genmove {opp_turn}")
    
    # Close the engine
    cmds.append("quit")
    
    # Join all commands into a single string to feed to the process
    gtp_input = "\n".join(cmds) + "\n"
    
    try:
        # Run the process and capture both stdout and stderr safely
        result = subprocess.run(
            ['mohex'], 
            input=gtp_input, 
            capture_output=True, 
            text=True
        )
        
        # 4. Extract the winrate from standard error
        winrate = 0.5
        
        # Check for guaranteed win/loss messages from MoHex's theorem prover
        if "Winning SC" in result.stderr or "is a winning move" in result.stderr:
            winrate = 1.0
        elif "Opponent has won" in result.stderr:
            winrate = 0.0
        else:
            # Strict regex: Only matches probabilities (0.0 to 1.0) or exact 0/1.
            matches = re.findall(r'(?i)(?:Score)\s*[:=]?\s*(1\.0+|0\.\d+|0|1)\b', result.stderr)
            if matches:
                # Grab the FIRST match (the Root's pure winrate) 
                winrate = float(matches[0])
            else:
                print("\n[WARNING] MoHex format unrecognized. Printing MoHex stderr for debugging:")
                print(result.stderr)
            
    except FileNotFoundError:
        print("CRITICAL ERROR: 'mohex' executable not found.")
        print("Please ensure MoHex is compiled and accessible in your PATH.")
        sys.exit(1)
    except Exception as e:
        print(f"Parsing error: {e}")
        winrate = 0.5
        
    return winrate

def main():
    if not os.path.exists("results_raw.csv") or not os.path.exists("dataset.csv"):
        print("Error: Missing CSV files. Run the C++ benchmark first.")
        sys.exit(1)

    print("Loading datasets...")
    df_results = pd.read_csv("results_raw.csv")
    df_states = pd.read_csv("dataset.csv")
    
    # Cache to avoid evaluating the same move on the same state multiple times
    # Key: (state_id, chosen_move_idx), Value: Oracle Score
    oracle_cache = {}
    
    scores = []
    total_evals = len(df_results)
    
    print("Evaluating C++ decisions with MoHex Oracle...")
    for index, row in df_results.iterrows():
        state_id = int(row['State_ID'])
        move_idx = int(row['Chosen_Move_Idx'])
        
        cache_key = (state_id, move_idx)
        
        if cache_key in oracle_cache:
            score = oracle_cache[cache_key]
        else:
            state_row = df_states.iloc[state_id].values
            score = evaluate_move(state_row, move_idx)
            oracle_cache[cache_key] = score
            
        scores.append(score)
        
        if (index + 1) % 10 == 0:
            print(f"Processed {index + 1}/{total_evals} evaluations...")
    
    df_results['Oracle_Score'] = scores
    df_results.to_csv("evaluated_results.csv", index=False)
    print("Evaluation complete. Data saved to evaluated_results.csv")
    
    # Generate the aggregated summary
    summary = df_results.groupby(['Config_ID', 'Agent', 'Sims', 'Threads']).agg(
        Avg_Oracle_Score=('Oracle_Score', 'mean'),
        Avg_Time_Per_Sim_Ms=('Time_Ms', 'mean')
    ).reset_index()
    
    summary.to_csv("evaluated_summary.csv", index=False)
    print("Summary saved to evaluated_summary.csv")

if __name__ == "__main__":
    main()