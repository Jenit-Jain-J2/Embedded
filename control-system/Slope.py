import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

# =========================================================
# 1. SETUP FOLDER & LOAD DATA
# =========================================================
output_folder = "graphs"
os.makedirs(output_folder, exist_ok=True)

try:
    df = pd.read_excel('calibration_data.xlsx', sheet_name='Sheet6')
    df.columns = df.columns.str.strip()
except Exception as e:
    print(f"Error reading file: {e}")
    exit()

# =========================================================
# 2. CALCULATE VARIABLES
# =========================================================
# Errors for Positive
err_vmon_pos = df['Vset+'] - df['Vmon+']
err_vmm_pos  = df['Vset+'] - df['Vmm+']

# Errors for Negative
err_vmon_neg = df['Vset-'] - df['Vmon-']
err_vmm_neg  = df['Vset-'] - df['Vmm-']

# Helper function for slope and equation text
def get_eq(x, y):
    slope, intercept = np.polyfit(x, y, 1)
    sign = "+" if intercept >= 0 else "-"
    eq_str = f'y = {slope:.5f}x {sign} {abs(intercept):.2f}'
    return eq_str

# =========================================================
# 3. SETUP THE 4x2 GRID
# =========================================================
fig, axs = plt.subplots(4, 2, figsize=(18, 26))
fig.suptitle('Voltage Tracking and Error Analysis (Sheet 6)', fontsize=24, fontweight='bold', y=0.96)
plt.subplots_adjust(hspace=0.35, wspace=0.2, top=0.93, bottom=0.05, left=0.08, right=0.95)

props = dict(boxstyle='round', facecolor='white', alpha=0.9, edgecolor='gray')

# =========================================================
# ROW 1: Vmon vs Vset
# =========================================================
# Positive
axs[0, 0].plot(df['Vset+'], df['Vmon+'], color='blue', marker='o', linestyle='-', label='Vmon+')
axs[0, 0].plot(df['Vset+'], df['Vset+'], color='red', linestyle=':', label='Perfect 1:1')
axs[0, 0].set_title('Positive: Vmon vs Vset', fontsize=15)
axs[0, 0].text(0.05, 0.95, get_eq(df['Vset+'], df['Vmon+']), transform=axs[0, 0].transAxes, fontsize=12, va='top', bbox=props)
axs[0, 0].legend(loc='lower right')
axs[0, 0].grid(True, alpha=0.3)
axs[0, 0].set_ylabel('Vmon (V)', fontsize=12)

# Negative
axs[0, 1].plot(df['Vset-'], df['Vmon-'], color='orange', marker='s', linestyle='-', label='Vmon-')
axs[0, 1].plot(df['Vset-'], df['Vset-'], color='red', linestyle=':', label='Perfect 1:1')
axs[0, 1].set_title('Negative: Vmon vs Vset', fontsize=15)
axs[0, 1].text(0.05, 0.95, get_eq(df['Vset-'], df['Vmon-']), transform=axs[0, 1].transAxes, fontsize=12, va='top', bbox=props)
axs[0, 1].legend(loc='lower right')
axs[0, 1].grid(True, alpha=0.3)

# =========================================================
# ROW 2: Vmm vs Vset
# =========================================================
# Positive
axs[1, 0].plot(df['Vset+'], df['Vmm+'], color='cyan', marker='o', linestyle='-', label='Vmm+')
axs[1, 0].plot(df['Vset+'], df['Vset+'], color='red', linestyle=':', label='Perfect 1:1')
axs[1, 0].set_title('Positive: Vmm vs Vset', fontsize=15)
axs[1, 0].text(0.05, 0.95, get_eq(df['Vset+'], df['Vmm+']), transform=axs[1, 0].transAxes, fontsize=12, va='top', bbox=props)
axs[1, 0].legend(loc='lower right')
axs[1, 0].grid(True, alpha=0.3)
axs[1, 0].set_ylabel('Vmm (V)', fontsize=12)

# Negative
axs[1, 1].plot(df['Vset-'], df['Vmm-'], color='magenta', marker='s', linestyle='-', label='Vmm-')
axs[1, 1].plot(df['Vset-'], df['Vset-'], color='red', linestyle=':', label='Perfect 1:1')
axs[1, 1].set_title('Negative: Vmm vs Vset', fontsize=15)
axs[1, 1].text(0.05, 0.95, get_eq(df['Vset-'], df['Vmm-']), transform=axs[1, 1].transAxes, fontsize=12, va='top', bbox=props)
axs[1, 1].legend(loc='lower right')
axs[1, 1].grid(True, alpha=0.3)

# =========================================================
# ROW 3: Vset - Vmon vs Vset
# =========================================================
# Positive
axs[2, 0].plot(df['Vset+'], err_vmon_pos, color='blue', marker='o', linestyle='-', label='Error')
axs[2, 0].axhline(y=0, color='red', linestyle=':', label='Zero Error')
axs[2, 0].set_title('Positive: (Vset+ - Vmon+) vs Vset+', fontsize=15)
axs[2, 0].text(0.05, 0.95, get_eq(df['Vset+'], err_vmon_pos), transform=axs[2, 0].transAxes, fontsize=12, va='top', bbox=props)
axs[2, 0].legend(loc='lower left')
axs[2, 0].grid(True, alpha=0.3)
axs[2, 0].set_ylabel('Error (V)', fontsize=12)

# Negative
axs[2, 1].plot(df['Vset-'], err_vmon_neg, color='orange', marker='s', linestyle='-', label='Error')
axs[2, 1].axhline(y=0, color='red', linestyle=':', label='Zero Error')
axs[2, 1].set_title('Negative: (Vset- - Vmon-) vs Vset-', fontsize=15)
axs[2, 1].text(0.05, 0.95, get_eq(df['Vset-'], err_vmon_neg), transform=axs[2, 1].transAxes, fontsize=12, va='top', bbox=props)
axs[2, 1].legend(loc='lower left')
axs[2, 1].grid(True, alpha=0.3)

# =========================================================
# ROW 4: Vset - Vmm vs Vset
# =========================================================
# Positive
axs[3, 0].plot(df['Vset+'], err_vmm_pos, color='green', marker='o', linestyle='-', label='Error')
axs[3, 0].axhline(y=0, color='red', linestyle=':', label='Zero Error')
axs[3, 0].set_title('Positive: (Vset+ - Vmm+) vs Vset+', fontsize=15)
axs[3, 0].text(0.05, 0.95, get_eq(df['Vset+'], err_vmm_pos), transform=axs[3, 0].transAxes, fontsize=12, va='top', bbox=props)
axs[3, 0].legend(loc='lower left')
axs[3, 0].grid(True, alpha=0.3)
axs[3, 0].set_xlabel('Vset+ (Commanded V)', fontsize=12)
axs[3, 0].set_ylabel('Error (V)', fontsize=12)

# Negative
axs[3, 1].plot(df['Vset-'], err_vmm_neg, color='purple', marker='s', linestyle='-', label='Error')
axs[3, 1].axhline(y=0, color='red', linestyle=':', label='Zero Error')
axs[3, 1].set_title('Negative: (Vset- - Vmm-) vs Vset-', fontsize=15)
axs[3, 1].text(0.05, 0.95, get_eq(df['Vset-'], err_vmm_neg), transform=axs[3, 1].transAxes, fontsize=12, va='top', bbox=props)
axs[3, 1].legend(loc='lower left')
axs[3, 1].grid(True, alpha=0.3)
axs[3, 1].set_xlabel('Vset- (Commanded V)', fontsize=12)

# =========================================================
# 4. SAVE & SHOW
# =========================================================
save_path = os.path.join(output_folder, "sheet_6_full_analysis.png")
plt.savefig(save_path, dpi=300, bbox_inches='tight')
print(f"\nGraph successfully saved to: {save_path}\n")

plt.show()