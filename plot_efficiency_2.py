#%%
import numpy as np
import matplotlib.pyplot as plt
import plotly.graph_objects as go
import plotly.io as pio
# # === Plot Style Config ===
plt.style.use('/home/esamouil/Downloads/pub_clean.mplstyle')

# True: fit GP in log10(E)-log10(efficiency) space.
# False: fit GP in linear E-efficiency space.
FIT_IN_LOG_SPACE = True

# Multiplies per-point measurement noise used by GP (alpha). >1 widens intervals.
NOISE_INFLATION = 1.0

# Add WhiteKernel term so GP can learn extra unmodeled noise.
USE_WHITE_KERNEL = True

# If True, calibrate GP sigma with leave-one-out residual spread.
CALIBRATE_SIGMA_WITH_LOO = True


def fit_gpr(
    energy_eV,
    hits,
    total,
    rng_seed=42,
    fit_in_log_space=FIT_IN_LOG_SPACE,
    noise_inflation=NOISE_INFLATION,
    use_white_kernel=USE_WHITE_KERNEL,
    calibrate_sigma_with_loo=CALIBRATE_SIGMA_WITH_LOO,
):
    """
    Fit efficiency(E) using Gaussian Process Regression either in log-space
    (log10(E), log10(efficiency)) or in linear-space (E, efficiency).

    Uncertainty is reported as the GP uncertainty on the mean prediction,
    with an optional global calibration factor from leave-one-out residuals.

    Returns:
      E_fine            : fine energy grid (eV)
      eps_central       : central fitted efficiency on E_fine
            eps_low, eps_high : 1-sigma band on E_fine
            central_model     : fitted GP model
            x_mean, x_std     : scaling parameters for GP input feature
            interval_scale    : multiplicative sigma calibration factor
    """
    try:
        from sklearn.gaussian_process import GaussianProcessRegressor
        from sklearn.gaussian_process.kernels import RBF, ConstantKernel, WhiteKernel
    except ImportError as exc:
        raise ImportError(
            "Gaussian regression requires scikit-learn. Install it with: pip install scikit-learn"
        ) from exc

    eps = np.clip(hits / total, 1e-12, 1 - 1e-12)
    sigma = np.sqrt(eps * (1 - eps) / total)
    sigma = np.clip(sigma, 1e-20, None)
    sigma_log = np.clip(sigma / (eps * np.log(10.0)), 1e-12, None)

    if fit_in_log_space:
        x_raw = np.log10(energy_eV).reshape(-1, 1)
        y = np.log10(eps)
        alpha = sigma_log**2
    else:
        x_raw = energy_eV.reshape(-1, 1)
        y = eps
        alpha = sigma**2

    alpha = np.clip((noise_inflation**2) * alpha, 1e-12, None)

    x_mean = float(np.mean(x_raw))
    x_std = float(np.std(x_raw))
    if x_std <= 0:
        x_std = 1.0
    x = (x_raw - x_mean) / x_std

    kernel = ConstantKernel(1.0, (1e-3, 1e3)) * RBF(length_scale=1.0, length_scale_bounds=(1e-2, 1e2))
    if use_white_kernel:
        kernel += WhiteKernel(noise_level=1e-4, noise_level_bounds=(1e-8, 1e-1))
    central_model = GaussianProcessRegressor(
        kernel=kernel,
        alpha=alpha,
        normalize_y=True,
        n_restarts_optimizer=5,
        random_state=rng_seed,
    )
    central_model.fit(x, y)

    E_fine = np.logspace(np.log10(np.min(energy_eV)), np.log10(np.max(energy_eV)), 600)
    if fit_in_log_space:
        x_fine_raw = np.log10(E_fine).reshape(-1, 1)
    else:
        x_fine_raw = E_fine.reshape(-1, 1)
    x_fine = (x_fine_raw - x_mean) / x_std
    y_central = central_model.predict(x_fine)
    if fit_in_log_space:
        eps_central = np.clip(10 ** y_central, 1e-12, 1 - 1e-12)
    else:
        eps_central = np.clip(y_central, 1e-12, 1 - 1e-12)

    y_std_central = central_model.predict(x_fine, return_std=True)[1]
    y_std_central = np.clip(y_std_central, 1e-12, None)

    # Calibrate sigma widths using leave-one-out standardized residual spread.
    interval_scale = 1.0
    if calibrate_sigma_with_loo:
        n = len(y)
        z_loo = np.empty(n)
        sigma_meas_fit = sigma_log if fit_in_log_space else sigma
        sigma_meas_fit = np.clip(noise_inflation * sigma_meas_fit, 1e-12, None)

        for i in range(n):
            mask = np.ones(n, dtype=bool)
            mask[i] = False

            x_tr_raw = x_raw[mask]
            y_tr = y[mask]
            a_tr = alpha[mask]

            x_m = float(np.mean(x_tr_raw))
            x_s = float(np.std(x_tr_raw))
            if x_s <= 0:
                x_s = 1.0

            x_tr = (x_tr_raw - x_m) / x_s
            x_te = (x_raw[i:i+1] - x_m) / x_s

            kernel_loo = ConstantKernel(1.0, (1e-3, 1e3)) * RBF(length_scale=1.0, length_scale_bounds=(1e-2, 1e2))
            if use_white_kernel:
                kernel_loo += WhiteKernel(noise_level=1e-4, noise_level_bounds=(1e-8, 1e-1))

            gp_loo = GaussianProcessRegressor(
                kernel=kernel_loo,
                alpha=np.clip(a_tr, 1e-12, None),
                normalize_y=True,
                n_restarts_optimizer=1,
                random_state=rng_seed + i,
            )
            gp_loo.fit(x_tr, y_tr)
            mu_i, std_i = gp_loo.predict(x_te, return_std=True)
            sigma_tot_i = np.sqrt(max(std_i[0], 1e-12) ** 2 + sigma_meas_fit[i] ** 2)
            z_loo[i] = (y[i] - mu_i[0]) / sigma_tot_i

        z_loo_std = float(np.std(z_loo, ddof=1)) if len(z_loo) > 1 else 1.0
        interval_scale = max(1.0, z_loo_std)

    y_std_eff = interval_scale * y_std_central
    if fit_in_log_space:
        eps_low = np.clip(10 ** (y_central - y_std_eff), 1e-12, 1 - 1e-12)
        eps_high = np.clip(10 ** (y_central + y_std_eff), 1e-12, 1 - 1e-12)
    else:
        eps_low = np.clip(y_central - y_std_eff, 1e-12, 1 - 1e-12)
        eps_high = np.clip(y_central + y_std_eff, 1e-12, 1 - 1e-12)

    print(f"GP fit space: {'log' if fit_in_log_space else 'linear'}")
    print(f"Central GP kernel: {central_model.kernel_}")
    print(f"Noise inflation: {noise_inflation:.3f} | Sigma scale: {interval_scale:.3f}")

    return (
        E_fine,
        eps_central,
        eps_low,
        eps_high,
        central_model,
        x_mean,
        x_std,
        fit_in_log_space,
        interval_scale,
    )


def efficiency_at_energy(
    E_eV,
    central_model,
    x_mean,
    x_std,
    fit_in_log_space=FIT_IN_LOG_SPACE,
    interval_scale=1.0,
):
    """Return efficiency and 1-sigma band at a single energy (or array)."""
    E_arr = np.atleast_1d(E_eV).astype(float)
    if fit_in_log_space:
        xq_raw = np.log10(E_arr).reshape(-1, 1)
    else:
        xq_raw = E_arr.reshape(-1, 1)
    xq = (xq_raw - x_mean) / x_std

    y_center = central_model.predict(xq)
    y_std = central_model.predict(xq, return_std=True)[1]
    y_std = np.clip(interval_scale * y_std, 1e-12, None)

    if fit_in_log_space:
        eps_center = np.clip(10 ** y_center, 1e-12, 1 - 1e-12)
        eps_low = np.clip(10 ** (y_center - y_std), 1e-12, 1 - 1e-12)
        eps_high = np.clip(10 ** (y_center + y_std), 1e-12, 1 - 1e-12)
    else:
        eps_center = np.clip(y_center, 1e-12, 1 - 1e-12)
        eps_low = np.clip(y_center - y_std, 1e-12, 1 - 1e-12)
        eps_high = np.clip(y_center + y_std, 1e-12, 1 - 1e-12)

    if np.isscalar(E_eV):
        return eps_center[0], eps_low[0], eps_high[0]
    return eps_center, eps_low, eps_high


def print_fit_quality_report(
    energy_eV,
    efficiency,
    unc,
    central_model,
    x_mean,
    x_std,
    fit_in_log_space=FIT_IN_LOG_SPACE,
    interval_scale=1.0,
    noise_inflation=NOISE_INFLATION,
    rng_seed=42,
):
    """Print fit-quality diagnostics for the GP model."""
    try:
        from sklearn.gaussian_process import GaussianProcessRegressor
        from sklearn.gaussian_process.kernels import RBF, ConstantKernel
    except ImportError:
        print("Fit quality report skipped: scikit-learn not available.")
        return

    if fit_in_log_space:
        x_raw = np.log10(energy_eV).reshape(-1, 1)
        y_obs = np.log10(np.clip(efficiency, 1e-12, 1 - 1e-12))
        sigma_meas = np.clip(unc / (np.clip(efficiency, 1e-12, 1 - 1e-12) * np.log(10.0)), 1e-12, None)
    else:
        x_raw = energy_eV.reshape(-1, 1)
        y_obs = np.clip(efficiency, 1e-12, 1 - 1e-12)
        sigma_meas = np.clip(unc, 1e-12, None)

    sigma_meas = np.clip(noise_inflation * sigma_meas, 1e-12, None)

    x = (x_raw - x_mean) / x_std

    # Central prediction with model uncertainty in log-space
    y_pred, y_std_model = central_model.predict(x, return_std=True)
    y_std_model = np.clip(y_std_model, 1e-12, None)

    sigma_total = np.sqrt(y_std_model**2 + sigma_meas**2)

    # Standardized residual diagnostics
    z = (y_obs - y_pred) / sigma_total
    z_mean = float(np.mean(z))
    z_std = float(np.std(z, ddof=1)) if len(z) > 1 else 0.0
    cov68_z = float(np.mean(np.abs(z) <= 1.0))
    cov95_z = float(np.mean(np.abs(z) <= 1.96))

    # Approximate reduced chi^2 using log-space uncertainties
    n = len(y_obs)
    p_eff = 3  # heuristic effective parameter count for reporting
    dof = max(1, n - p_eff)
    chi2 = float(np.sum(((y_obs - y_pred) / sigma_total) ** 2))
    chi2_red = chi2 / dof

    # Leave-one-out cross-validation
    loo_y_pred = np.empty(n)
    loo_y_std = np.empty(n)
    for i in range(n):
        mask = np.ones(n, dtype=bool)
        mask[i] = False

        x_tr_raw = x_raw[mask]
        y_tr = y_obs[mask]
        s_tr = sigma_meas[mask]

        x_m = float(np.mean(x_tr_raw))
        x_s = float(np.std(x_tr_raw))
        if x_s <= 0:
            x_s = 1.0

        x_tr = (x_tr_raw - x_m) / x_s
        x_te = (x_raw[i:i+1] - x_m) / x_s

        kernel = ConstantKernel(1.0, (1e-3, 1e3)) * RBF(length_scale=1.0, length_scale_bounds=(1e-2, 1e2))
        gp_loo = GaussianProcessRegressor(
            kernel=kernel,
            alpha=np.clip(s_tr**2, 1e-12, None),
            normalize_y=True,
            n_restarts_optimizer=1,
            random_state=rng_seed + i,
        )
        gp_loo.fit(x_tr, y_tr)
        mu_i, std_i = gp_loo.predict(x_te, return_std=True)
        loo_y_pred[i] = mu_i[0]
        loo_y_std[i] = max(std_i[0], 1e-12)

    loo_rmse = float(np.sqrt(np.mean((y_obs - loo_y_pred) ** 2)))
    loo_z = (y_obs - loo_y_pred) / np.sqrt(loo_y_std**2 + sigma_meas**2)
    loo_z_mean = float(np.mean(loo_z))
    loo_z_std = float(np.std(loo_z, ddof=1)) if len(loo_z) > 1 else 0.0

    print("\n=== GP Fit Quality Report ===")
    print(f"Fit space: {'log' if fit_in_log_space else 'linear'}")
    print(f"Noise inflation: {noise_inflation:.3f} | Interval scale: {interval_scale:.3f}")
    print(f"Standardized residuals: mean={z_mean:.3f}, std={z_std:.3f}")
    print(f"Reduced chi^2 (approx): {chi2_red:.3f}  (chi2={chi2:.2f}, dof={dof})")
    print(f"Coverage (consistent z-space): |z|<=1 -> {100*cov68_z:.1f}%, |z|<=1.96 -> {100*cov95_z:.1f}%")
    rmse_label = "log10 efficiency" if fit_in_log_space else "efficiency"
    print(f"LOO RMSE ({rmse_label}): {loo_rmse:.4f}")
    print(f"LOO standardized residuals: mean={loo_z_mean:.3f}, std={loo_z_std:.3f}")


#%%
data = np.loadtxt("run_2/efficiency.txt", comments="#")

energy = data[:, 0]            # eV
efficiency = data[:, 1]
hits = data[:, 2].astype(int)
total = data[:, 3].astype(int)
unc = np.sqrt(efficiency * (1 - efficiency) / total)  # binomial sigma

# Raw points plot
fig, ax = plt.subplots(figsize=(8, 5))
ax.errorbar(energy, efficiency, yerr=unc, fmt="o", markersize=4, linewidth=1.2, capsize=3)
ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("Neutron Energy (eV)", fontsize=13)
ax.set_ylabel("Efficiency", fontsize=13)
ax.set_title("Detection Efficiency vs Neutron Energy", fontsize=14)
#ax.grid(True, which="both", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("efficiency_plot.png", dpi=150)
plt.show()
print("Saved efficiency_plot.png")


#%%
# Fit + uncertainty band
E_fine, eps_fit, eps_low, eps_high, central_model, x_mean, x_std, fit_used, interval_scale = fit_gpr(
    energy,
    hits,
    total,
    rng_seed=42,
    fit_in_log_space=FIT_IN_LOG_SPACE,
    noise_inflation=NOISE_INFLATION,
    use_white_kernel=USE_WHITE_KERNEL,
    calibrate_sigma_with_loo=CALIBRATE_SIGMA_WITH_LOO,
)

print_fit_quality_report(
    energy,
    efficiency,
    unc,
    central_model,
    x_mean,
    x_std,
    fit_in_log_space=fit_used,
    interval_scale=interval_scale,
    noise_inflation=NOISE_INFLATION,
    rng_seed=42,
)

fig, ax = plt.subplots(figsize=(8, 5))
ax.errorbar(
    energy,
    efficiency,
    yerr=unc,
    fmt="o",
    markersize=4,
    linewidth=0,
    elinewidth=1.2,
    capsize=3,
    label="Simulation",
)
ax.plot(E_fine, eps_fit, linewidth=1.8, linestyle="--", label="Gaussian Process fit")
ax.fill_between(E_fine, eps_low, eps_high, alpha=0.25, label="Fit uncertainty (68%)")

ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("Neutron Energy (eV)", fontsize=13)
ax.set_ylabel("Efficiency", fontsize=13)
ax.set_title("Efficiency Fit (Gaussian Process) with Uncertainty Band", fontsize=14)
ax.legend(fontsize=10)
#ax.grid(True, which="both", linestyle="--", alpha=0.5)

plt.tight_layout()
plt.savefig("efficiency_plot_fit.png", dpi=150)
plt.show()
print("Saved efficiency_plot_fit.png")


#%%
# Interactive plot (zoom/pan) shown inline in VS Code/Jupyter
fig_int = go.Figure()

fig_int.add_trace(
    go.Scatter(
        x=energy,
        y=efficiency,
        mode="markers",
        name="Simulation",
        error_y=dict(type="data", array=unc, visible=True),
        marker=dict(size=6),
    )
)

fig_int.add_trace(
    go.Scatter(
        x=E_fine,
        y=eps_fit,
        mode="lines",
        name="Gaussian Process fit",
        line=dict(width=2, dash="dash"),
    )
)

# Uncertainty band (68%)
fig_int.add_trace(
    go.Scatter(
        x=E_fine,
        y=eps_high,
        mode="lines",
        line=dict(width=0),
        showlegend=False,
        hoverinfo="skip",
    )
)
fig_int.add_trace(
    go.Scatter(
        x=E_fine,
        y=eps_low,
        mode="lines",
        fill="tonexty",
        fillcolor="rgba(100, 100, 255, 0.25)",
        line=dict(width=0),
        name="Fit uncertainty (68%)",
        hoverinfo="skip",
    )
)

fig_int.update_layout(
    title="Efficiency Fit (Gaussian Process) with Uncertainty Band (Interactive)",
    xaxis_title="Neutron Energy (eV)",
    yaxis_title="Efficiency",
    template="plotly_white",
)
fig_int.update_xaxes(type="log")
fig_int.update_yaxes(type="log")

try:
    pio.renderers.default = "vscode"
except Exception:
    pio.renderers.default = "notebook_connected"

fig_int.show()
print("Displayed interactive Plotly figure inline (no HTML file written).")


#%%
# Example query at a single energy (edit E_query_eV as needed)
E_query_eV = 0.3
eps_q, eps_q_low, eps_q_high = efficiency_at_energy(
    E_query_eV,
    central_model,
    x_mean,
    x_std,
    fit_in_log_space=fit_used,
    interval_scale=interval_scale,
)
print(
    f"Efficiency at E={E_query_eV:.6g} eV: "
    f"{eps_q:.6e}  (68% band: [{eps_q_low:.6e}, {eps_q_high:.6e}])"
)


#%%
# Static plot with points, error bars, and fitted curve only (no fit-uncertainty band)
fig, ax = plt.subplots(figsize=(8, 5))
ax.errorbar(
    energy,
    efficiency,
    yerr=unc,
    fmt="o",
    markersize=4,
    linewidth=0,
    elinewidth=1.2,
    capsize=3,
    label="Simulation",
)
ax.plot(E_fine, eps_fit, linewidth=1.8, linestyle="--", label="Gaussian Process fit")

ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("Neutron Energy (eV)", fontsize=13)
ax.set_ylabel("Efficiency", fontsize=13)
ax.set_title(r"Efficiency Fit For 100 nm wide pure $\mathbf{{}^{10}B_4C}$ layer", fontsize=14)
ax.legend(fontsize=10)

#ax.grid(True, which="both", linestyle=":", linewidth=0.7)
plt.tight_layout()
plt.savefig("efficiency_plot_fit_static.png", dpi=150)
plt.show()
print("Saved efficiency_plot_fit_static.png")


#%%
# Same plot as above but with wavelength on the x-axis
# lambda (Angstrom) = 0.2865 / sqrt(E [eV])
wavelength = 0.2865 / np.sqrt(energy)
lambda_fine = 0.2865 / np.sqrt(E_fine)

# E_fine is ascending -> lambda_fine is descending; sort to ascending wavelength
sort_idx = np.argsort(lambda_fine)
lambda_fine_s = lambda_fine[sort_idx]
eps_fit_s = eps_fit[sort_idx]
eps_low_s = eps_low[sort_idx]
eps_high_s = eps_high[sort_idx]

fig, ax = plt.subplots(figsize=(8, 5))
ax.errorbar(
    wavelength,
    efficiency,
    yerr=unc,
    fmt="o",
    markersize=4,
    linewidth=0,
    elinewidth=1.2,
    capsize=3,
    label="Simulation",
)
ax.plot(lambda_fine_s, eps_fit_s, linewidth=1.8, linestyle="--", label="Gaussian Process fit")


#ax.set_xscale("log")
ax.set_xlim(0, 20)
ax.set_yscale("log")
ax.set_xlabel(r"Neutron Wavelength ($\AA$)", fontsize=13)
ax.set_ylabel("Efficiency", fontsize=13)
ax.set_title(r"Efficiency Fit For 100 nm wide pure $\mathbf{{}^{10}B_4C}$ layer", fontsize=14)
ax.legend(fontsize=10)
ax.grid(True, which="both", linestyle=":", linewidth=0.7)

plt.tight_layout()
plt.savefig("efficiency_plot_fit_wavelength.png", dpi=150)
plt.show()
print("Saved efficiency_plot_fit_wavelength.png")



# %%
