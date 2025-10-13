import asyncio  
import RPi.GPIO as GPIO  
from bleak import BleakClient  
import struct 

GPIO.setwarnings(False) 


LED_PIN = 17  

GPIO.setmode(GPIO.BCM)  
GPIO.setup(LED_PIN, GPIO.OUT)  

pwm = GPIO.PWM(LED_PIN, 1000)  
pwm.start(0)  


ARDUINO_MAC = "94:b5:55:c0:84:9a"


def map_range(value, in_min, in_max, out_min=0, out_max=100):
    
    value = max(in_min, min(in_max, value))
    
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

# function to connect to Arduino and handle BLE communication
async def connect_and_listen():
    print(f"Attempting to connect to {ARDUINO_MAC}") 
    
    try:
      
        async with BleakClient(ARDUINO_MAC, timeout=60.0) as client:  
            print("Successfully connected!")            
            await asyncio.sleep(2.0) # Wait for connection to stabilize
            
           
            services = client.services
            print("Available services:")
           
            for service in services:
                print(f"Service: {service.uuid}")
                for char in service.characteristics:
                    print(f"  Characteristic: {char.uuid} - Properties: {char.properties}")
                    
                  
                    if "notify" in char.properties:
                        print(f"Using characteristic: {char.uuid}")
                        
            
                        def notification_handler(sender, data):
                            try:
                                # Convert 4-byte data to float 
                                lux = struct.unpack('f', data)[0]
                                print(f"Light: {lux:.2f} lx")
                                
                                brightness = map_range(lux, 1, 100, 100, 0)  
                                
                                # Update LED brightness using PWM duty cycle
                                pwm.ChangeDutyCycle(brightness)
                                print(f"LED Brightness: {brightness:.1f}% (Closer object = Brighter LED)")
                                
                            # exception/error handling
                            except Exception as e:
                                print(f"Data error: {e}")
                        
                        # Enable notifications for this characteristic
                        await client.start_notify(char.uuid, notification_handler)
                        print("Notifications started! System is working.")
                        print("Move your hand CLOSER to sensor Ã¢â€ â€™ LED gets BRIGHTER")
                        print("Move your hand AWAY from sensor Ã¢â€ â€™ LED gets DIMMER")
                        print("Press Ctrl+C to stop.")
                        
                        # Keep the program running indefinitely
                        while True:
                            await asyncio.sleep(1)
                
                print("No suitable characteristics found")
                
    except Exception as e:
        print(f"Connection failed: {e}")
        import traceback
        traceback.print_exc()  # Print detailed error info
    finally:
       
        pwm.ChangeDutyCycle(0)

if __name__ == "__main__":
    try:
        # Start the BLE communication
        asyncio.run(connect_and_listen())
        
    # Handle Ctrl+C gracefully
    except KeyboardInterrupt:
        print("Stopped by user")
        
    finally:
        pwm.stop()  # Stop PWM
        GPIO.cleanup()  # Reset GPIO pins
        print("Cleanup complete")