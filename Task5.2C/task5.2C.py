import tkinter as tk
import RPi.GPIO as GPIO

# GPIO setup
GPIO.setmode(GPIO.BOARD)
leds = [11, 13, 15]   
for pin in leds:
    GPIO.setup(pin, GPIO.OUT)

# Setup PWM
p1 = GPIO.PWM(11, 100); p1.start(0)
p2 = GPIO.PWM(13, 100); p2.start(0)
p3 = GPIO.PWM(15, 100); p3.start(0)

# Functions to control brightness
def led1(val): p1.ChangeDutyCycle(int(val))
def led2(val): p2.ChangeDutyCycle(int(val))
def led3(val): p3.ChangeDutyCycle(int(val))

# GUI
win = tk.Tk()
win.title("LED Control")

tk.Scale(win, from_=0, to=100, orient="vertical", label="LED 1", command=led1).pack()
tk.Scale(win, from_=0, to=100, orient="vertical", label="LED 2", command=led2).pack()
tk.Scale(win, from_=0, to=100, orient="vertical", label="LED 3", command=led3).pack()

def exit_program():
    p1.stop(); p2.stop(); p3.stop()
    GPIO.cleanup()
    win.destroy()

tk.Button(win, text="Exit", command=exit_program).pack()
win.mainloop()
