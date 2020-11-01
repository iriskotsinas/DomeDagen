# DomeDagen

A multi-player game made for the dome at Visualiseringscenter C. Using [SGCT-Networked-Application](https://github.com/alexanderbock/SGCT-Networked-Application).

To clone this repository, don't forget to recurse submodules: `git clone --recurse-submodules https://github.com/iriskotsinas/DomeDagen.git` or the dependent libraries will not work correctly.

# Getting started
1. Execute `npm install` in the `webserver` folder
2. Compile the application
3. Start the web server with `node server.js` in the `webserver` folder
4. Start the application
5. Connect from a second device

In the current version, it is necessary to start the server *before* starting the application, as the application will only try to connect once at startup.

# Configurations
Currently, the server and application addresses are encoded in several places that all have to be changed:
1. Create `config.json` in `/webserver` without comments

    ```
    {
        "serverAddress":"ws://192.168.0.103",   //ip to your local server
        "gameAddress":"::ffff:127.0.0.1",       //ip to gameserver localhost when on your own machine
        "gamePort":"8081",                      //port to gameserver
        "serverPort":"8081"                     //port to your local server
    } 
    ```

2. The ip and port at which the game application will try to connect to to the webserver is set in `config.ini` in the root directory, ex:

    ```ini
    [Network]
    ip = localhost
    port = 81
    ```

    
