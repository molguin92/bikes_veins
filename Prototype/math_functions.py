import math


def estimated_accel(v, dv, real_gap, a=5.0, u=16.7,
                    preceding_car=True):
    # u = 60km/h = 16.6...m/s

    def desired_gap(d0=2.0, T=0.8, b=3.0):
        if preceding_car:
            return d0 + (T * v) + ((v * dv) / (2 * math.sqrt(a * b)))
        else:
            return 0

    x = math.pow((v / u), 4)
    y = math.pow(desired_gap()/real_gap, 2)

    return a * (1 - x - y)
