import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import sys
import os

def load_and_aggregate_data(filepath, mode):
    if not os.path.exists(filepath):
        print(f"\n[!] Error: {filepath} not found.")
        return None
        
    df_raw = pd.read_csv(filepath)
    
    if mode == 'Match':
        df = df_raw.groupby(['Config_ID', 'Agent', 'Sims', 'Threads']).agg(
            Test_Time_Sec=('Test_Time_Sec', 'sum'),
            Test_Turns=('Test_Turns', 'sum'),
            Global_Winrate=('Test_Won', lambda x: x.mean() * 100)
        ).reset_index()
        
        # Calculate derived metrics safely
        df['Avg_Time_Per_Turn'] = (df['Test_Time_Sec'] / df['Test_Turns']).fillna(0)
        
        # Time per 1k Sims: (Total Time / (Total Turns * Sims)) * 1,000,000 to convert sec -> ms per 1k
        df['Avg_Time_Per_1k_Sims_Ms'] = ((df['Test_Time_Sec'] / (df['Test_Turns'] * df['Sims'])) * 1000000).fillna(0)
        
    else:
        df = df_raw.groupby(['Config_ID', 'Agent', 'Sims', 'Threads']).agg(
            Avg_Oracle_Score=('Oracle_Score', 'mean'),
            Time_Ms=('Time_Ms', 'sum'),
            Eval_Count=('Time_Ms', 'count')
        ).reset_index()
        
        # Time per 1k Sims: (Total Time ms / (Count * Sims)) * 1000
        df['Avg_Time_Per_1k_Sims_Ms'] = ((df['Time_Ms'] / (df['Eval_Count'] * df['Sims'])) * 1000).fillna(0)
    
    # Calculate Relative Speedup dynamically
    df['Relative_Speedup'] = 0.0
    for sims in df['Sims'].unique():
        baseline_df = df[(df['Sims'] == sims) & (df['Agent'] == 'Sequential')]
        if not baseline_df.empty:
            baseline_time = baseline_df['Avg_Time_Per_1k_Sims_Ms'].values[0]
            mask = df['Sims'] == sims
            # Protect against division by zero if time is incredibly fast
            if baseline_time > 0:
                df.loc[mask, 'Relative_Speedup'] = baseline_time / df.loc[mask, 'Avg_Time_Per_1k_Sims_Ms']
            
    return df

def plot_common_graphs(df, choice, sub_choice_val, mode):
    plt.figure(figsize=(10, 6))
    sns.set_theme(style="whitegrid")
    
    if choice == '1':
        print("\n--- Plot Description ---")
        print("Visualizing: Execution Time per 1k Simulations vs. Thread Count.")
        subset = df[(df['Sims'] == sub_choice_val) & (df['Agent'] != 'Sequential')]
        sns.lineplot(data=subset, x='Threads', y='Avg_Time_Per_1k_Sims_Ms', hue='Agent', marker='o', linewidth=2, markersize=8)
        plt.title(f'Time per 1k Simulations vs Threads ({sub_choice_val} Sims)', fontsize=14, pad=15)
        plt.ylabel('Time per 1k Simulations (ms)', fontsize=12)
        plt.xlabel('Threads', fontsize=12)
        plt.xticks([2, 3, 4])

    elif choice == '2':
        print("\n--- Plot Description ---")
        print("Visualizing: Parallel Speedup vs. Thread Count.")
        subset = df[(df['Sims'] == sub_choice_val) & (df['Agent'] != 'Sequential')]
        sns.lineplot(data=subset, x='Threads', y='Relative_Speedup', hue='Agent', marker='o', linewidth=2, markersize=8)
        plt.plot([2, 3, 4], [2, 3, 4], 'k--', label='Ideal Linear Speedup', alpha=0.6)
        plt.title(f'Relative Speedup vs Threads ({sub_choice_val} Sims)', fontsize=14, pad=15)
        plt.ylabel('Relative Speedup (x)', fontsize=12)
        plt.xlabel('Threads', fontsize=12)
        plt.xticks([2, 3, 4])
        plt.legend()
        

def handle_match_menu(df):
    sim_options = sorted(df['Sims'].unique())
    thread_options = sorted(df[df['Agent'] != 'Sequential']['Threads'].unique())
    
    while True:
        print("\n" + "-"*40)
        print("        MATCH PIPELINE METRICS        ")
        print("-"*40)
        print("1. Time per 1k Sims vs Threads")
        print("2. Relative Speedup vs Threads")
        print("3. Global Winrate vs Threads")
        print("4. Pareto Front (Time per Turn vs Winrate)")
        print("5. Learning Curve (Winrate vs Sims Budget)")
        print("6. Back to Main Menu")
        
        choice = input("\nSelect a plot (1-6): ").strip()
        if choice == '6': break
        if choice not in ['1', '2', '3', '4', '5']: continue

        if choice in ['1', '2', '3', '4']:
            print("\n--- Select Simulation Budget ---")
            for i, sim in enumerate(sim_options, 1): print(f"{i}. {sim} Sims")
            try:
                sim_idx = int(input(f"Select budget (1-{len(sim_options)}): ")) - 1
                sub_val = sim_options[sim_idx]
            except (ValueError, IndexError): continue

            if choice in ['1', '2']:
                plot_common_graphs(df, choice, sub_val, 'Match')
            elif choice == '3':
                plt.figure(figsize=(10, 6))
                sns.set_theme(style="whitegrid")
                subset = df[(df['Sims'] == sub_val) & (df['Agent'] != 'Sequential')]
                sns.lineplot(data=subset, x='Threads', y='Global_Winrate', hue='Agent', marker='o', linewidth=2, markersize=8)
                plt.title(f'Global Winrate vs Threads ({sub_val} Sims)', fontsize=14, pad=15)
                plt.ylabel('Global Winrate (%)', fontsize=12)
                plt.xlabel('Threads', fontsize=12)
                plt.xticks([2, 3, 4])
            elif choice == '4':
                plt.figure(figsize=(10, 6))
                sns.set_theme(style="whitegrid")
                subset = df[(df['Sims'] == sub_val)]
                sizes = {1: 50, 2: 100, 3: 150, 4: 200, 8: 350}
                sns.scatterplot(data=subset, x='Avg_Time_Per_Turn', y='Global_Winrate', hue='Agent', size='Threads', sizes=sizes, alpha=0.8)
                plt.title(f'Pareto Front: Efficiency vs Strength ({sub_val} Sims)', fontsize=14, pad=15)
                plt.ylabel('Global Winrate (%)', fontsize=12)
                plt.xlabel('Average Time Per Turn (Seconds)', fontsize=12)

        elif choice == '5':
            print("\n--- Select Thread Count ---")
            for i, t in enumerate(thread_options, 1): print(f"{i}. {t} Threads")
            try:
                t_idx = int(input(f"Select thread count (1-{len(thread_options)}): ")) - 1
                sub_val = thread_options[t_idx]
            except (ValueError, IndexError): continue
            
            plt.figure(figsize=(10, 6))
            sns.set_theme(style="whitegrid")
            subset = df[(df['Threads'] == sub_val) | (df['Agent'] == 'Sequential')]
            sns.lineplot(data=subset, x='Sims', y='Global_Winrate', hue='Agent', marker='o', linewidth=2, markersize=8)
            plt.title(f'Learning Curve: Winrate vs Simulations ({sub_val} Threads)', fontsize=14, pad=15)
            plt.ylabel('Global Winrate (%)', fontsize=12)
            plt.xlabel('Total Simulations', fontsize=12)

        print("\n[!] Opening Graph Window... Close the graph window to return.")
        plt.show()

def handle_oracle_menu(df):
    sim_options = sorted(df['Sims'].unique())
    thread_options = sorted(df[df['Agent'] != 'Sequential']['Threads'].unique())
    
    while True:
        print("\n" + "-"*40)
        print("       ORACLE PIPELINE METRICS        ")
        print("-"*40)
        print("1. Time per 1k Sims vs Threads")
        print("2. Relative Speedup vs Threads")
        print("3. Oracle Score vs Threads")
        print("4. Pareto Front (Time per 1k Sims vs Oracle Score)")
        print("5. Learning Curve (Oracle Score vs Sims Budget)")
        print("6. Back to Main Menu")
        
        choice = input("\nSelect a plot (1-6): ").strip()
        if choice == '6': break
        if choice not in ['1', '2', '3', '4', '5']: continue

        if choice in ['1', '2', '3', '4']:
            print("\n--- Select Simulation Budget ---")
            for i, sim in enumerate(sim_options, 1): print(f"{i}. {sim} Sims")
            try:
                sim_idx = int(input(f"Select budget (1-{len(sim_options)}): ")) - 1
                sub_val = sim_options[sim_idx]
            except (ValueError, IndexError): continue

            if choice in ['1', '2']:
                plot_common_graphs(df, choice, sub_val, 'Oracle')
            elif choice == '3':
                plt.figure(figsize=(10, 6))
                sns.set_theme(style="whitegrid")
                subset = df[(df['Sims'] == sub_val) & (df['Agent'] != 'Sequential')]
                sns.lineplot(data=subset, x='Threads', y='Avg_Oracle_Score', hue='Agent', marker='o', linewidth=2, markersize=8)
                plt.title(f'Oracle Score vs Threads ({sub_val} Sims)', fontsize=14, pad=15)
                plt.ylabel('Average Oracle Score', fontsize=12)
                plt.xlabel('Threads', fontsize=12)
                plt.xticks([2, 3, 4])
            elif choice == '4':
                plt.figure(figsize=(10, 6))
                sns.set_theme(style="whitegrid")
                subset = df[(df['Sims'] == sub_val)]
                sizes = {1: 50, 2: 100, 3: 150, 4: 200, 8: 350}
                sns.scatterplot(data=subset, x='Avg_Time_Per_1k_Sims_Ms', y='Avg_Oracle_Score', hue='Agent', size='Threads', sizes=sizes, alpha=0.8)
                plt.title(f'Pareto Front: Efficiency vs Strength ({sub_val} Sims)', fontsize=14, pad=15)
                plt.ylabel('Average Oracle Score', fontsize=12)
                plt.xlabel('Time Per 1k Simulations (ms)', fontsize=12)

        elif choice == '5':
            print("\n--- Select Thread Count ---")
            for i, t in enumerate(thread_options, 1): print(f"{i}. {t} Threads")
            try:
                t_idx = int(input(f"Select thread count (1-{len(thread_options)}): ")) - 1
                sub_val = thread_options[t_idx]
            except (ValueError, IndexError): continue
            
            plt.figure(figsize=(10, 6))
            sns.set_theme(style="whitegrid")
            subset = df[(df['Threads'] == sub_val) | (df['Agent'] == 'Sequential')]
            sns.lineplot(data=subset, x='Sims', y='Avg_Oracle_Score', hue='Agent', marker='o', linewidth=2, markersize=8)
            plt.title(f'Learning Curve: Oracle Score vs Sims ({sub_val} Threads)', fontsize=14, pad=15)
            plt.ylabel('Average Oracle Score', fontsize=12)
            plt.xlabel('Total Simulations', fontsize=12)

        print("\n[!] Opening Graph Window... Close the graph window to return.")
        plt.show()

def main():
    while True:
        print("\n" + "="*45)
        print("         MCTS BENCHMARK VISUALIZER         ")
        print("="*45)
        print("1. Analyze Match Pipeline Data (match_raw.csv)")
        print("2. Analyze Oracle Pipeline Data (oracle_raw.csv)")
        print("3. Exit")
        
        choice = input("\nSelect data source (1-3): ").strip()
        
        if choice == '1':
            df = load_and_aggregate_data("match_raw.csv", 'Match')
            if df is not None: handle_match_menu(df)
        elif choice == '2':
            df = load_and_aggregate_data("oracle_raw.csv", 'Oracle')
            if df is not None: handle_oracle_menu(df)
        elif choice == '3':
            print("Exiting...")
            break
        else:
            print("Invalid choice. Please enter 1-3.")

if __name__ == "__main__":
    main()