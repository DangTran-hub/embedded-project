# MQTT API

Firmware uses the following base topic:

```text
smartlock/<lock_id>
```

## Status Topic

```text
smartlock/<lock_id>/status
```

Example lock status payload:

```json
{
  "locked": true,
  "online": true,
  "battery": 86,
  "method": "boot",
  "by": "He Thong",
  "save": false
}
```

Example pending RFID payload:

```json
{
  "pending_id": "63CDA256",
  "online": true
}
```

## Command Topic

```text
smartlock/<lock_id>/cmd
```

Unlock from app:

```json
{
  "action": "unlock",
  "by": "Mobile App"
}
```

Start RFID learning mode:

```json
{
  "action": "START_LEARNING"
}
```

Add RFID card:

```json
{
  "action": "ADD_CARD",
  "id": "63CDA256"
}
```

Remove RFID card:

```json
{
  "action": "REMOVE_CARD",
  "id": "63CDA256"
}
```
