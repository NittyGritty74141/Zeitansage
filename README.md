# Zeitansage
The telephone time announcement of the Deutsche Bundespost greatly reduced

Long time ago, this was the most precise clock. You had to dial '119' and got into the loop. There is an old video on YT
https://www.youtube.com/watch?v=BAaR1WlnShM
where you get explanation about how this huge machine is working.

Inspired by the Timespeak Sketch - you will see the rest of it in the code ...

I found the complete announcements in one Video
https://www.youtube.com/watch?v=iJkdPHfVqCI

I extracted it to MP3 and uses Audacity to split off the huge file into useful audio snippets.

Finally, I modified the code (not yet perfect, but working) and created another "Clock" - compared to the rack in the video
it is now just a loudspeaker, a DF-Player and ESP8266 (I used NodeMCU Board, but Wemos and others will also work.

Just to mention the 1k-Resistor to be put into Rx and Tx lines - I have no clue, why, but without, the DF-Player does behave weired.

According to the above video (with three pickups) - I placed the audio files into 3 folders on the SD-Card. Just see the code, you will understand.

Don't forget to also create a folder mp3 and advert on the SD-Card for the DF-Player
Connections from ESP to DF-Player 
DF-Player - ESP
Tx   1k   - D7
Rx   1k   - D6
Busy      - D5

For more details on how to connect ESP8266 and DF-Player, contact Google :-)

One issue I found is, that the first announcement is not correct - but all following are.
Also there is an issue after pressing the RESET Button, operation stops for some reason (no audio output)
This device is just doing contiues speak - like the original - until it is powered :-)
After a while, you would like to hangup the phone - as in the early days - just unplug the power
