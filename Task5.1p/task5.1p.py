import tkinter as tk
import RPi.GPIO as GPIO

# --- GPIO setup ---
GPIO.setmode(GPIO.BOARD)   # Use PHYSICAL pin numbers
RED = 29
GREEN = 31
RED2 = 33
PINS = [RED, GREEN, RED2]

for p in PINS:
    GPIO.setup(p, GPIO.OUT)
    GPIO.output(p, GPIO.LOW)

# --- Functions to control LEDs ---
def set_led(choice):
    for pin in PINS:
        GPIO.output(pin, GPIO.LOW)

    if choice == 1:
        GPIO.output(RED, GPIO.HIGH)
    elif choice == 2:
        GPIO.output(GREEN, GPIO.HIGH)
    elif choice == 3:
        GPIO.output(RED2, GPIO.HIGH)

def on_exit():
    for p in PINS:
        GPIO.output(p, GPIO.LOW)
    GPIO.cleanup()
    root.destroy()

# --- Tkinter GUI ---
root = tk.Tk()
root.title("LED Selector")
root.geometry("500x300")

label = tk.Label(root, text="Select LED to turn ON:")
label.pack(pady=10)

choice = tk.IntVar()

tk.Radiobutton(root, text="Red", variable=choice, value=1,
               command=lambda: set_led(1)).pack()
tk.Radiobutton(root, text="Green", variable=choice, value=2,
               command=lambda: set_led(2)).pack()
tk.Radiobutton(root, text="Red2", variable=choice, value=3,
               command=lambda: set_led(3)).pack()

exit_btn = tk.Button(root, text="Exit", command=on_exit)
exit_btn.pack(pady=15)

root.protocol("WM_DELETE_WINDOW", on_exit)
root.mainloop()
