# HotWater

HotWater is a project for Embedded Sistems Course at Fauldade de Ciências that have the goal to create a system to control an eletric water boiler.

This repository have all the files and information needed to measure data from the water boiler. For this i used an arduino with one relay, one temperature sensor, one sensor to measure the water and one relay.

## Comunication

The arduino is listening to the port 5070 for UDP datagrams.

All comunication, like orders or reports, is made with codes (e.g. "A00"). All the codes are documented below.

The Sketch for arduino input and output comunication is different. You can see how to make the datagrams in the section below.

### Incoming Comunications

The incoming messages should always follow this sketch:

`%%%CODE%%%`

For this project the Arduino only receives orders, never receiving arguments. Consequently all orders have only one code for the requested order.

### Outcoming Comunications

The outcoming messages should always follow this sketch

`%%%CODE%%%STATUS%%%+++VALUE+++(VALUE)%%%`

This is how arduino will reply to an incoming comunication. The `CODE` is the same of the incoming message. The `STATUS` is the status of the order/request. The `VALUE` is the output of the order, this output can be a list of values instead of only one `VALUE`.


### Logs

The log messages should always follow this sketch:

`%%%CODE%%%+++VALUE+++(VALUE)%%%`

The arduino can send logs to a IP without reveiving any order. The Sketch is almost the same as the one for outcoming messages, except for the `VALUE` key wich does not exist in this context


### Codes
All documentation should be made by codes. 

The code for all comunications can be examined below:

#### Orders

- ##### A01 (Relay On)

    Order to turn on the Relay
    
    ###### Request
    
    CODE: A00
    
    ###### Reply
    
    CODE | STATUS | VALUE | Description
    :---:  | :---: | :---: | :--- 
    A00 | AAA | 000 | Success
    A00 | FFF | AF1 | Failure: Relay already on
    A00 | FFF | AF2 | Failuer: Whater already max temperature
    A00 | FFF | AF3 | Failuer: Not enought water
    A00 | FFF | AFF | Failuer: Uknown Error
    
- ##### B01 (Relay Off)

    Order to turn off the Relay
    
    ##### Request
    
    CODE: B00
    
    ##### Reply
    
    CODE | STATUS | VALUE | Description
    :---:  | :---: | :---: | :--- 
    B00 | AAA | 000 | Success
    B00 | FFF | BF1 | Failure: Relay already off
    B00 | FFF | BFF | Failuer: Uknown Error
    
## IMPORTANT

This project is still under development and this standards can change on any given time.
