# analysis.py

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# data
data = pd.read_csv('log.csv', index_col = 0, parse_dates = True)
data.index = pd.to_datetime(data.index * 1000, unit = 'ms')


# times
data['td'] = data.index.to_series().diff()
times = []
for i in range(len(data)):
    if data['log'].iloc[i] == 'dataReceived':
        times.append(data['td'][i].microseconds)
times.sort()

# analysis
print('average:', np.mean(times))
print('std:', np.std(times))

# plot
plt.hist(times, bins = 100)
plt.show()
