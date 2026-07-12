import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import sys
import os

def load_data(filepath="plot_data.csv"):
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found in the current directory.")
        sys.exit(1)
        
    df = pd.read_csv(filepath)
    
    # Calculate Relative Speedup dynamically
    df['Relative_Speedup'] = 0.0
    for sims in df['Sims'].unique():
        baseline_df = df[(df['Sims'] == sims) & (df['Agent'] == 'Sequential')]
        if not baseline_df.empty:
            baseline_time = baseline_df['Avg_Time_Per_Sim_Ms'].values[0]
            mask = df['Sims'] == sims
            df.loc[mask, 'Relative_Speedup'] = baseline_time / df.loc[mask, 'Avg_Time_Per_Sim_Ms']
            
    return df

def plot_graph(df, choice, sub_choice_val):
    plt.figure(figsize=(10, 6))
    sns.set_theme(style="whitegrid")
    
    if choice == '1':
        # 1. Time per Simulation vs Threads (filtered by Sims)
        print("\n--- Plot Description ---")
        print("Visualizing: Raw Execution Time vs. Thread Count.")
        print("Context: Evaluates the physical scaling limits of the implementations.")
        print("Look for: Flattening curves that indicate hardware saturation, lock contention ")
        print("(e.g., in Lock-Free or Root), or tree-traversal bottlenecks as thread counts increase.")
        
        subset = df[(df['Sims'] == sub_choice_val) & (df['Agent'] != 'Sequential')]
        sns.lineplot(data=subset, x='Threads', y='Avg_Time_Per_Sim_Ms', hue='Agent', marker='o', linewidth=2, markersize=8)
        
        plt.title(f'Time per Simulation vs Threads ({sub_choice_val} Sims)', fontsize=14, pad=15)
        plt.ylabel('Time per Simulation (ms)', fontsize=12)
        plt.xlabel('Threads', fontsize=12)
        plt.xticks([2, 4, 8])

    elif choice == '2':
        # 2. Relative Speedup vs Threads (filtered by Sims)
        print("\n--- Plot Description ---")
        print("Visualizing: Parallel Speedup vs. Thread Count.")
        print("Context: Maps directly to Amdahl's Law for the MCTS loop.")
        print("Look for: Deviation from the ideal linear scaling line. Leaf parallelization usually ")
        print("scales poorly due to lack of shared tree info, while Virtual Loss might show overhead costs.")
        
        subset = df[(df['Sims'] == sub_choice_val) & (df['Agent'] != 'Sequential')]
        sns.lineplot(data=subset, x='Threads', y='Relative_Speedup', hue='Agent', marker='o', linewidth=2, markersize=8)
        plt.plot([2, 4, 8], [2, 4, 8], 'k--', label='Ideal Linear Speedup', alpha=0.6)
        
        plt.title(f'Relative Speedup vs Threads ({sub_choice_val} Sims)', fontsize=14, pad=15)
        plt.ylabel('Relative Speedup (x)', fontsize=12)
        plt.xlabel('Threads', fontsize=12)
        plt.xticks([2, 4, 8])
        plt.legend()

    elif choice == '3':
        # 3. Global Winrate vs Threads (filtered by Sims)
        print("\n--- Plot Description ---")
        print("Visualizing: Playing Strength vs. Thread Count.")
        print("Context: Tracks search degradation (Search Overhead). Parallel MCTS inherently ")
        print("loses exploitation efficiency as threads explore independently before backpropagating.")
        print("Look for: Severe winrate drops at higher thread counts, particularly in lock-free ")
        print("or root parallelization where race conditions or tree splitting dilute path values.")
        
        subset = df[(df['Sims'] == sub_choice_val) & (df['Agent'] != 'Sequential')]
        sns.lineplot(data=subset, x='Threads', y='Global_Winrate', hue='Agent', marker='o', linewidth=2, markersize=8)
        
        plt.title(f'Global Winrate vs Threads ({sub_choice_val} Sims)', fontsize=14, pad=15)
        plt.ylabel('Global Winrate (%)', fontsize=12)
        plt.xlabel('Threads', fontsize=12)
        plt.xticks([2, 4, 8])

    elif choice == '4':
        # 4. Pareto Front (filtered by Sims)
        print("\n--- Plot Description ---")
        print("Visualizing: Pareto Efficiency (Time per Turn vs. Global Winrate).")
        print("Context: The ultimate deployment metric. Combines computational speed with playing strength.")
        print("Look for: Configurations closest to the top-left corner (fastest moves, highest winrate). ")
        print("Agents in the bottom-right are strictly dominated (slow and weak).")
        
        subset = df[(df['Sims'] == sub_choice_val)]
        # Map threads to marker sizes for the scatter plot
        sizes = {1: 50, 2: 100, 4: 200, 8: 350}
        
        sns.scatterplot(
            data=subset, x='Avg_Time_Per_Turn', y='Global_Winrate', 
            hue='Agent', size='Threads', sizes=sizes, alpha=0.8
        )
        
        plt.title(f'Pareto Front: Efficiency vs Strength ({sub_choice_val} Sims)', fontsize=14, pad=15)
        plt.ylabel('Global Winrate (%)', fontsize=12)
        plt.xlabel('Average Time Per Turn (Seconds)', fontsize=12)

    elif choice == '5':
        # 5. Winrate vs Sims (filtered by Thread count)
        print("\n--- Plot Description ---")
        print("Visualizing: Convergence Rate (Winrate vs. Simulation Budget).")
        print("Context: Evaluates the learning curve of the algorithms.")
        print("Look for: Agents that plateau early, indicating that higher simulation budgets are ")
        print("wasted due to parallel search inefficiencies (e.g., getting stuck in local optima).")
        
        # Include the sequential baseline for comparison against the selected thread count
        subset = df[(df['Threads'] == sub_choice_val) | (df['Agent'] == 'Sequential')]
        sns.lineplot(data=subset, x='Sims', y='Global_Winrate', hue='Agent', marker='o', linewidth=2, markersize=8)
        
        plt.title(f'Learning Curve: Winrate vs Simulations ({sub_choice_val} Threads)', fontsize=14, pad=15)
        plt.ylabel('Global Winrate (%)', fontsize=12)
        plt.xlabel('Total Simulations', fontsize=12)
        plt.xticks([5000, 10000, 20000])

    print("\n[!] Opening Graph Window...")
    print("[!] Close the graph window to return to the menu.")
    plt.show()

def main():
    df = load_data()
    sim_options = sorted(df['Sims'].unique())
    thread_options = sorted(df[df['Agent'] != 'Sequential']['Threads'].unique())
    
    while True:
        print("\n" + "="*45)
        print("   MCTS HEX BENCHMARK VISUALIZER   ")
        print("="*45)
        print("1. Time per Simulation vs Threads")
        print("2. Relative Speedup vs Threads")
        print("3. Global Winrate vs Threads")
        print("4. Pareto Front (Time per Turn vs Winrate)")
        print("5. Learning Curve (Winrate vs Sims Budget)")
        print("6. Exit")
        
        choice = input("\nSelect a plot (1-6): ").strip()
        
        if choice == '6':
            print("Exiting...")
            break
            
        if choice not in ['1', '2', '3', '4', '5']:
            print("Invalid choice. Please enter 1-6.")
            continue
            
        # Choices 1-4 require filtering by Simulation Budget
        if choice in ['1', '2', '3', '4']:
            print("\n--- Select Simulation Budget ---")
            for i, sim in enumerate(sim_options, 1):
                print(f"{i}. {sim} Sims")
                
            sub_choice = input(f"Select budget (1-{len(sim_options)}): ").strip()
            
            try:
                sim_idx = int(sub_choice) - 1
                if 0 <= sim_idx < len(sim_options):
                    plot_graph(df, choice, sim_options[sim_idx])
                else:
                    print("Invalid budget selection.")
            except ValueError:
                print("Please enter a valid number.")
                
        # Choice 5 requires filtering by Thread Count
        elif choice == '5':
            print("\n--- Select Thread Count ---")
            for i, t in enumerate(thread_options, 1):
                print(f"{i}. {t} Threads")
                
            sub_choice = input(f"Select thread count (1-{len(thread_options)}): ").strip()
            
            try:
                t_idx = int(sub_choice) - 1
                if 0 <= t_idx < len(thread_options):
                    plot_graph(df, choice, thread_options[t_idx])
                else:
                    print("Invalid thread selection.")
            except ValueError:
                print("Please enter a valid number.")

if __name__ == "__main__":
    main()