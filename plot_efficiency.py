#%%
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import UnivariateSpline
import plotly.graph_objects as go
import plotly.io as pio


def fit_loglog_spline_with_bootstrap(
    energy_eV,
    hits,
    total,
    n_boot=1000,
    rng_seed=42,
    smooth_factor_scale=1.0,
):
    """
    Fit efficiency(E) in log-log space and estimate fit uncertainty with bootstrap.

    Returns:
      E_fine            : fine energy grid (eV)
      eps_central       : central fitted efficiency on E_fine
      eps_low, eps_high : 16th/84th percentile band on E_fine
      boot_splines      : bootstrap spline objects for arbitrary-energy queries
      central_spline    : central spline object
    """
    eps = hits / total
    # Avoid zeros/ones for log + weighting stability
    eps = np.clip(eps, 1e-12, 1 - 1e-12)

    # Binomial uncertainty and propagated log10 uncertainty
    sigma = np.sqrt(eps * (1 - eps) / total)
    sigma = np.clip(sigma, 1e-20, None)
    sigma_log = sigma / (eps * np.log(10.0))
    sigma_log = np.clip(sigma_log, 1e-12, None)

    x = np.log10(energy_eV)
    y = np.log10(eps)
    w = 1.0 / sigma_log

    # Weighted smoothing spline in log-log space.
    # s~N gives a true fit (not point-to-point interpolation) while still honoring weights.
    s_val = max(0.0, smooth_factor_scale) * len(x)
    central_spline = UnivariateSpline(x, y, w=w, s=s_val)

    E_fine = np.logspace(np.min(x), np.max(x), 600)
    x_fine = np.log10(E_fine)
    eps_central = 10 ** central_spline(x_fine)

    # Bootstrap over binomial counting stats at each energy point
    rng = np.random.default_rng(rng_seed)
    boot_curves = np.empty((n_boot, len(E_fine)))
    boot_splines = []

    for i in range(n_boot):
        k_boot = rng.binomial(total.astype(int), eps)
        eps_boot = np.clip(k_boot / total, 1e-12, 1 - 1e-12)

        sigma_boot = np.sqrt(eps_boot * (1 - eps_boot) / total)
        sigma_boot = np.clip(sigma_boot, 1e-20, None)
        sigma_boot_log = sigma_boot / (eps_boot * np.log(10.0))
        sigma_boot_log = np.clip(sigma_boot_log, 1e-12, None)

        y_boot = np.log10(eps_boot)
        w_boot = 1.0 / sigma_boot_log

        spline_boot = UnivariateSpline(x, y_boot, w=w_boot, s=s_val)
        boot_splines.append(spline_boot)
        boot_curves[i, :] = 10 ** spline_boot(x_fine)

    eps_low = np.percentile(boot_curves, 16, axis=0)
    eps_high = np.percentile(boot_curves, 84, axis=0)

    return E_fine, eps_central, eps_low, eps_high, boot_splines, central_spline


def efficiency_at_energy(E_eV, central_spline, boot_splines):
    """Return efficiency and 1-sigma band at a single energy (or array)."""
    E_arr = np.atleast_1d(E_eV).astype(float)
    xq = np.log10(E_arr)

    eps_center = 10 ** central_spline(xq)
    eps_boot = np.array([10 ** sp(xq) for sp in boot_splines])
    eps_low = np.percentile(eps_boot, 16, axis=0)
    eps_high = np.percentile(eps_boot, 84, axis=0)

    if np.isscalar(E_eV):
        return eps_center[0], eps_low[0], eps_high[0]
    return eps_center, eps_low, eps_high


#%%
data = np.loadtxt("build/efficiency.txt", comments="#")

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
ax.grid(True, which="both", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("efficiency_plot.png", dpi=150)
plt.show()
print("Saved efficiency_plot.png")


#%%
# Fit + uncertainty band
E_fine, eps_fit, eps_low, eps_high, boot_splines, central_spline = fit_loglog_spline_with_bootstrap(
    energy,
    hits,
    total,
    n_boot=1000,
    rng_seed=42,
    smooth_factor_scale=1.0,
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
ax.plot(E_fine, eps_fit, linewidth=1.8, linestyle="--", label="Spline fit")
ax.fill_between(E_fine, eps_low, eps_high, alpha=0.25, label="Fit uncertainty (68%)")

ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("Neutron Energy (eV)", fontsize=13)
ax.set_ylabel("Efficiency", fontsize=13)
ax.set_title("Efficiency Fit with Uncertainty Band", fontsize=14)
ax.legend(fontsize=10)
ax.grid(True, which="both", linestyle="--", alpha=0.5)

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
        name="Spline fit",
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
    title="Efficiency Fit with Uncertainty Band (Interactive)",
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
eps_q, eps_q_low, eps_q_high = efficiency_at_energy(E_query_eV, central_spline, boot_splines)
print(
    f"Efficiency at E={E_query_eV:.6g} eV: "
    f"{eps_q:.6e}  (68% band: [{eps_q_low:.6e}, {eps_q_high:.6e}])"
)


# %%
