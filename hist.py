#%%
import matplotlib.pyplot as plt
#%%
# Read numbers from the text file
numbers = []
with open("numbers.txt", "r") as f:
    for line in f:
        try:
            numbers.append(float(line.strip()))
        except ValueError:
            pass  # ignore empty or invalid lines
#%%
# Plot histogram
plt.hist(numbers, bins=100, color='skyblue', edgecolor='black')
plt.xlabel('Energy (MeV)')
plt.ylabel('Counts')
plt.title('Energy Deposited in Gas Volume per Event')
plt.grid(True)
plt.savefig("histogram.png")
plt.show()
# %%
