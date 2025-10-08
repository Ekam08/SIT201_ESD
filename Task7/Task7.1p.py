mport tkinter as tk
from gpiozero import LED
import speech_recognition as sr
from threading import Thread

# Initialize LED
led = LED(18)

# Initialize recognizer
recognizer = sr.Recognizer()
mic = sr.Microphone()

# Create GUI
root = tk.Tk()
root.title("Voice Controlled LED")

status_label = tk.Label(root, text="LED Status: OFF", font=("Helvetica", 18))
status_label.pack(padx=20, pady=20)

# Function to update GUI
def update_status(text):
    status_label.config(text=f"LED Status: {text}")

# Function to listen to voice commands
def listen_commands():
    while True:
        try:
            with mic as source:
                recognizer.adjust_for_ambient_noise(source)
                print("Listening...")
                audio = recognizer.listen(source)

            command = recognizer.recognize_google(audio).lower()
            print("You said:", command)

            if "on" in command:
                led.on()
                update_status("ON")
            elif "off" in command:
                led.off()
                update_status("OFF")

        except sr.UnknownValueError:
            print("Could not understand audio")
        except sr.RequestError:
            print("Network error")

# Run the listening function in a separate thread
listener_thread = Thread(target=listen_commands, daemon=True)
listener_thread.start()

# Run the GUI
root.mainloop()
