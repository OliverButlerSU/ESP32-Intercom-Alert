mqtt_username="USERNAME"
mqtt_password="PASSWORD"

# Generate a Client ID with the subscribe prefix.
client_id = 'Discord Bot'
port = XXX
broker = "XXX.XXX.XXX.XXX"
door_alert_topic = "esp32/Door_Alert"
door_ack_topic = "esp32/Door_ACK"
door_open_topic = "esp32/Door_Open"

DISC_TOKEN = "Discord Token"




import discord
from discord.ext import commands
from paho.mqtt import client as mqtt_client
import threading
import asyncio
import time




print("Connecting to Discord")
intents = discord.Intents.all()
discord_client = commands.Bot(command_prefix='!', intents=intents)





def mqtt_thread():
    def on_connect(client, userdata, flags, rc):
            if rc == 0:
                print("Connected to MQTT Broker!")
                client.subscribe(door_alert_topic)
                client.subscribe(door_ack_topic)
            else:
                print("Failed to connect, return code %d\n", rc)

    def on_message(client, userdata, msg):
        message = msg.payload.decode()
        topic = msg.topic
        print(f"Received MQTT message from topic: '{topic}': {message}")
        
        # Fetch all guilds (servers) the bot is in
        for guild in discord_client.guilds:
    

            # Fetch all channels in the guild
            for channel in guild.channels:
                # Check if channel is a text channel and its name contains "door-alert"
                if isinstance(channel, discord.TextChannel) and 'door-alert' in channel.name:
                    if topic == door_alert_topic:
                        # Send message to the channel
                        asyncio.run_coroutine_threadsafe(channel.send("Someone has rung your door"), discord_client.loop)
                    elif topic == door_ack_topic:
                        asyncio.run_coroutine_threadsafe(channel.send("Your door has been opened"), discord_client.loop)

    mt_client = mqtt_client.Client(client_id)
    mt_client.username_pw_set(mqtt_username, mqtt_password)
    mt_client.on_connect = on_connect
    mt_client.on_message = on_message
    mt_client.connect(broker, port)
    mt_client.loop_forever()

def publish_message(topic, message):
    mt_client = mqtt_client.Client(client_id)
    mt_client.username_pw_set(mqtt_username, mqtt_password)
    mt_client.connect(broker, port)
    mt_client.publish(topic, message)
    print("Publishing message")
    mt_client.disconnect()
    


@discord_client.event
async def on_ready():
    print("Connected to Discord")
    mt_thread = threading.Thread(target=mqtt_thread)
    mt_thread.start()
    print("Started thread")

@discord_client.event
async def on_message(message):
    print()
    if message.content.startswith("!opendoor") and message.channel.name.startswith("door-alert"):
        print("Publishing message")
        publish_message(door_open_topic, time.ctime())
        await message.channel.send("Requesting door to be open")    

def run_discord():
    discord_client.run(DISC_TOKEN)

run_discord()