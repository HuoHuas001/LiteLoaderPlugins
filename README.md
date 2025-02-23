# LiteLoaderPlugins
<a href="https://github.com/LiteLDev/LiteLoaderPlugins/actions">![status](https://img.shields.io/github/workflow/status/LiteLDev/LiteLoaderPlugins/Build%20LiteLoaderPlugins?style=for-the-badge)</a>
<a href="https://t.me/liteloader">![Telegram](https://img.shields.io/badge/telegram-LiteLoader-%232CA5E0?style=for-the-badge&logo=Telegram)</a>  
English | [简体中文](README_zh-cn.md)  
Some example plugins for [LiteLoader](https://github.com/LiteLDev/BDSLiteLoader)

# Usage
## LLMoney
| Command | Description | Permission |
| --- | --- | --- |
| /money query [player] | Query your/other players's balance | Player/OP |
| /money pay <player> <amount> | Transfer money to a player | Player |
| /money set <player> <amount> | Set player's balance | OP |
| /money add <player> <amount> | Add player's balance | OP |
| /money reduce <player> <amount> | Reduce player's balance | OP |
| /money hist | Print your running account | Player |
| /money purge | Clear your running account | OP |
| /money_s | Command with TargetSelector | Player |
## LLHelper
| Command | Description | Permission |
| --- | --- | --- |
| /gmode <player> <pode(0/1/2/3)> | Change gamemode | OP |
| /ban <ban/unban/list> [player] [time(second)] | Ban/unban a player | OP |
| /transfer <player> <IP> <port> | Transfer a player to another server | OP |
| /helper <reload/update> | Reload config/Check plugin's update | OP |
| /skick <player> | Force kick a player from server | OP |
| /cname <set/rm> <player> [nickname] | Set nickname for a player | OP |
| /vanish | invisibility(only effective for players who can see you) | OP |
| /crash <player> | Crash player's client | OP |
| /runas <player> <command> | Execute command as target player | OP |
## LLTpa
| Command | Description | Permission |
| --- | --- | --- |
| /tpa <to/here/ac/de/cancel/toggle> [player]	| Teleport to a player's location/teleport the player to your location/accept the teleport/refuse the teleport/cancel the teleport/change the status of whether to accept the teleport | Player |
| /warp <add/del/go/ls> [warp] | Add a warp/delete warp/go to warp/list warp | OP |
| /home <add/del/go/ls> [home] | Add a home/delete home/go home/list home | Player |
| /item | Show the item info on hand | Player |
| /lltpa <reload/update> [isForce: true or false] | Reload config/Check plugin's update | OP |

# Config
## LLHelper
```json
{
    "language": "en-us",
    "force_enable_expplay": false,//Enable experimental mode
    "commmand_map": {//Use item to click block to execute a command
        "391": "me 114514"
    },
    "timer": {//Timed loop execution command
        "600": "say Welcome to the server"
    },
    "ban_items": [],//Prohibited items, use ID
    "log_items": [//Items that will be recorded in the log after use, use ID
        7
    ],
    "force_enable_ability": false,//Turn on the /ability command, if the educational version is turned on, the server will be abnormal
    "fake_seed": 114514,//Fake seed
    "no_explosion": true,//Stop all explosions in the server
    "max_chat_length": 1919,//Maximum length of chat message
    "log_chat": true,//Log chat
    "log_cmd": true,//Log command
    "no_enderman_take_block": true,//Stop the enderman from picking up the block
    "protect_farm_block": true//Protect arable land from being trampled on
```
## LLtpa
```json
{
    "language": "en-us",
    "max_homes": 3,//The maximum number of homes the player can set
    "tpa_timeout": 20000,//tpa request timeout
    "tpa_ratelimit": 5000,//tpa request cooldown
    "back_enabled": true,//Enable /back command
    "suicide_enabled": true//Enable /suicide command
}
```
## LLMoney
```json
{
    "language": "en-us",
    "def_money": 0, //Default money
    "pay_tax": 0.0 //Tax
}
```
