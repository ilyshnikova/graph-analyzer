import numpy as np
from sklearn.svm import SVC
import json
import csv
import sys

data = json.loads(sys.argv[1])
ans = json.loads(sys.argv[2])

cl = SVC(kernel='linear')
cl.fit(data, ans)


print(json.dumps({'blocks_coeffs' : list(cl.coef_[0]), 'shift_coeff' : cl.intercept_[0]}))
