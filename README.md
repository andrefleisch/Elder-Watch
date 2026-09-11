# Elder Watch

A device that stays with an elderly person and notifies the family through Telegram when something happens. It detects falls on its own, has an emergency button for manual triggering, and also works as a medication reminder.

The idea came from a simple problem: when an older person falls at home alone, the time until someone notices can be what separates a scare from something serious. This project tries to shorten that time.

---

## What it does

**Detects falls automatically.** The motion sensor tracks acceleration and body position continuously. When it identifies the characteristic pattern of a fall, it fires the alert without anyone having to do anything.

**Has a panic button.** If the person feels unwell, dizzy, or unsafe, a single press sends the warning immediately.

**Listens to the room.** A sound sensor checks whether there was a loud noise at the moment of the fall, such as a shout or the impact itself. That information goes along with the message and helps whoever receives it judge how serious the situation is.

**Notifies through Telegram.** The message arrives on the caregiver's phone with the type of event and the exact time.

**Reminds about medication.** You can register up to five alarms with a name and a time. At the scheduled time, an LED lights up and the buzzer sounds for a few seconds.

**Shows everything on a web page.** Just open the device's address in a browser to see a real-time motion chart, the record of the last fall, and the list of alarms.

---

## How it recognizes a fall

The challenge here is telling a real fall apart from any sudden movement. The device uses two different paths for that.

The first mimics what physically happens during a fall: for a fraction of a second the body is in free fall and the sensor registers very low acceleration; right after comes the impact against the floor, a sharp spike. It is the **combination of the two in the right sequence** that confirms the fall. If the free fall happens but the impact doesn't follow shortly after, the alert is cancelled, because it was only a quick movement.

The second path is tilt. If the device detects that the person is lying down or heavily tilted and stays that way for several seconds, it assumes they may have fallen and been unable to get up.

Before any decision, the sensor data passes through a filter that smooths the readings. This prevents an isolated vibration from being mistaken for an accident.

---

## What you need to build it

- An **ESP32** board (it does everything and already includes Wi-Fi)
- An **MPU6050** motion sensor
- A sound sensor
- A buzzer and two LEDs
- A button

The pinout for each component and the sensitivity values are commented at the top of the code, along with the required libraries.

---

## Getting it running

1. Assemble the circuit following the pins indicated in the code.
2. Fill in your Wi-Fi network name and password.
3. Create a Telegram bot by talking to **@BotFather** and paste the token and your chat ID into the code.
4. Flash the program onto the board and open the serial monitor to see the IP address that appears.
5. Type that address into the browser of any phone or computer on the same network.

If Wi-Fi doesn't connect within fifteen seconds, the board restarts on its own and tries again.

> Passwords and the token are written directly in the code. Before publishing the repository, make sure they have been removed. And if the real token was ever pushed at any point, generate a new one through BotFather.

---

## Tuning the sensitivity

The thresholds that define what counts as a fall are at the very beginning of the file, all grouped together and commented. Lowering the impact value makes the device more sensitive, but it also increases the chance of false alarms. It's worth testing with the device in the real position it will be used in, since the response changes considerably depending on where it's attached to the body.

---

## Limitations

Some honest notes about the current state:

- **Alarms are lost if the board restarts.** They are kept only in volatile memory.
- **The device goes "deaf" for a few seconds after an alert.** During that window the chart freezes and a second fall would not be detected.
- **Location doesn't appear in the message.** The code does look up the approximate position, but it ends up not being included in the text that gets sent.
- **The position would be imprecise anyway.** The lookup is done through the internet address, which points to the provider's region rather than where the person actually is. For real-world use, a GPS module would be necessary.
- **The web page has no password.** Anyone connected to the same network can create or delete alarms.
