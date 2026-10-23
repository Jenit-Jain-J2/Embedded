import numpy as np
import matplotlib.pyplot as plt

# =========================================================
# 1. ENTER YOUR DATA HERE
# Paste your measured values into these lists.
# Make sure all lists have the same number of items.
# =========================================================

# The voltage you commanded the software to set (e.g., 100, 200, 300...)
vset_cmd = np.array([100.0, 200.0, 300.0, 400.0, 500.0])

# The voltage shown on your TFT display (Vmon)
vmon_tft = np.array([104.09, 203.39, 304.73, 404.83, 503.82])

# The ACTUAL voltage measured by your calibrated multimeter / HV probe (Vout)
vout_true = np.array([108.4, 209.5, 313.4, 412.5, 515.0])

# The current shown on your TFT display (Imon)
imon_tft = np.array([6.79, 14.01, 21.22, 28.11, 35.26])

# The true resistance of your load in MegaOhms (MΩ)
R_true_mohm = np.array([10.0, 10.0, 10.0, 10.0, 10.0])

# Your current factors in config.h
CURRENT_HV_FACTOR = 1192.5860
CURRENT_IMON_FACTOR = 50.0

# =========================================================
# 2. CALCULATIONS
# =========================================================

print("--- CALIBRATION RESULTS ---")

# A. Calculate True Current from Ohms Law (I = V/R)
# Since V is in Volts and R is in MΩ, I is in µA.
i_true = vout_true / R_true_mohm

# B. Voltage Correction Factor (Vmon)
# We want: Vmon_new = Vout_true
# Multiplier = Vout_true / Vmon_tft
v_multipliers = vout_true / vmon_tft
avg_v_multiplier = np.mean(v_multipliers)
new_hv_factor = CURRENT_HV_FACTOR * avg_v_multiplier

print(f"\nVoltage Calibration:")
print(f"  Average Correction Multiplier : {avg_v_multiplier:.6f}")
print(f"  NEW CH1_HV_FACTOR to use      : {new_hv_factor:.4f}")

# C. Current Correction Factor (Imon)
# We want: Imon_new = I_true
# Multiplier = I_true / Imon_tft
i_multipliers = i_true / imon_tft
avg_i_multiplier = np.mean(i_multipliers)
new_imon_factor = CURRENT_IMON_FACTOR * avg_i_multiplier

print(f"\nCurrent Calibration:")
print(f"  Average Correction Multiplier : {avg_i_multiplier:.6f}")
print(f"  NEW CH1_IMON_FACTOR to use    : {new_imon_factor:.4f}")

# D. Vset Tracking (DAC linearity)
# This checks if the DAC needs a mathematical slope/intercept
slope, intercept = np.polyfit(vset_cmd, vout_true, 1)
print(f"\nVset to Vout Tracking:")
print(f"  Vout = {slope:.6f} * Vset + {intercept:.6f}")
if abs(slope - 1.0) > 0.01:
    print(f"  WARNING: The DAC slope is off by {(1-slope)*100:.2f}%")
else:
    print(f"  DAC slope is perfect.")

# =========================================================
# 3. PLOTS
# =========================================================
plt.figure(figsize=(10, 4))

plt.subplot(1, 2, 1)
plt.plot(vout_true, vmon_tft, 'o', label='Displayed Vmon')
plt.plot(vout_true, vout_true, 'r--', label='Perfect 1:1')
plt.xlabel("True Vout (Multimeter)")
plt.ylabel("Displayed Vmon")
plt.title("Voltage Calibration")
plt.legend()
plt.grid(True)

plt.subplot(1, 2, 2)
plt.plot(i_true, imon_tft, 'o', label='Displayed Imon')
plt.plot(i_true, i_true, 'r--', label='Perfect 1:1')
plt.xlabel("True Current (µA)")
plt.ylabel("Displayed Imon (µA)")
plt.title("Current Calibration")
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()