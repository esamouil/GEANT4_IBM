#%%
import math

#%%

a=0.0001
b=5

Nsteps = 30

loga=math.log10(a)
logb=math.log10(b)

for i in range(Nsteps):
    logx = loga + (logb - loga) * i / (Nsteps - 1)
    x = 10 ** logx
    print(f"{x:.4f}")

# %%
