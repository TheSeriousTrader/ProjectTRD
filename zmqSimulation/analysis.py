# analysis.py

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# data

data = pd.read_csv('log.csv', index_col = 0, parse_dates = True)

data.index = pd.to_datetime(data.index * 1000, unit = 'ms')


# ms

data['td'] = data.index.to_series().diff()

ms = []

for i in range(len(data)):

    if data['log'].iloc[i] == 'dataReceived':
        
        if data['td'][i].microseconds < 200:
            
            ms.append(data['td'][i].microseconds)
 
ms.sort()

print(' ')
print('average:', np.mean(ms))
print('std:', np.std(ms))

plt.hist(ms, bins = 100)
plt.show()