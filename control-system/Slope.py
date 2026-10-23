import pandas as pd
import matplotlib.pyplot as plt

# =========================================================
# 1. LOAD THE DATA FROM EXCEL
# =========================================================
try:
    # Read the Excel file (make sure openpyxl is installed)
    df = pd.read_excel('calibration_data.xlsx')
    # Remove any accidental spaces in column headers
    df.columns = df.columns.str.strip()
except Exception as e:
    print(f"Error reading file: {e}")
    print("Make sure the file is named 'calibration_data.xlsx' and 'openpyxl' is installed.")
    exit()

# =========================================================
# 2. PLOT THE GRAPHS (2 Plots side-by-side)
# =========================================================
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
fig.suptitle('Voltage vs Current (V/I) Characteristics', fontsize=16, fontweight='bold')

# --- Graph 1: Positive Channel ---
ax1.plot(df['Imon+'], df['Vset+'], marker='o', linestyle='-', color='blue', label='Vset+ (Target)')
ax1.plot(df['Imon+'], df['Vmon+'], marker='s', linestyle='--', color='red', label='Vmon+ (Actual)')

ax1.set_title('Positive Channel')
ax1.set_xlabel('Current Imon+ (µA)')
ax1.set_ylabel('Voltage (V)')
ax1.legend()
ax1.grid(True, alpha=0.3)

# --- Graph 2: Negative Channel ---
ax2.plot(df['Imon-'], df['Vset-'], marker='o', linestyle='-', color='green', label='Vset- (Target)')
ax2.plot(df['Imon-'], df['Vmon-'], marker='s', linestyle='--', color='orange', label='Vmon- (Actual)')

ax2.set_title('Negative Channel')
ax2.set_xlabel('Current Imon- (µA)')
ax2.set_ylabel('Voltage (V)')
ax2.legend()
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.show()