import sympy as sp
import numpy as np
import matplotlib.pyplot as plt

class Melody:
    def __init__ (self, scale=1, n=5, coeff_range=(-3,3)):
        self.f = self.generate_trig(n, coeff_range) * scale

    def generate_trig(self, n, coeff_range):
        """
        Generate a random sinusoidal shape of melody.
        """
        x = sp.Symbol('x')
        a0 = np.random.randint(*coeff_range)
        a = np.random.randint(*coeff_range, n)
        b = np.random.randint(*coeff_range, n)
        expr = a0 + sum(a[k]*sp.cos((k+1)*x) + b[k]*sp.sin((k+1)*x) for k in range(n))
        return sp.simplify(expr)
    
    def shift_right(self, shift_val=1):
        """
        Shift self.f right or left.
        """
        x = sp.Symbol('x')
        if self.f is not None:
            self.f = self.f.subs(x, x - shift_val)

    def shift_up(self, shift_val=1):
        """
        Shift self.f up or down.
        """
        if self.f is not None:
            self.f = self.f + shift_val

    def plot_function(self):
        """
        Plot function just to visualize.
        """
        x_vals = np.linspace(-2*np.pi, 2*np.pi, 400)
        f_lambdified = sp.lambdify(sp.Symbol('x'), self.f, modules=['numpy'])
        y_vals = f_lambdified(x_vals)

        plt.plot(x_vals, y_vals)
        plt.xlabel('x')
        plt.ylabel('f(x)')
        plt.title('Random Trig Function')
        plt.grid(True)

    def __repr__(self):
        return f"f(x) = {self.f}"


m = Melody(0.1, 4)
m.plot_function()
n = Melody(1, 4)
n.plot_function()
o = Melody(-1, 4)
o.shift_up(2)
o.plot_function()
plt.show()