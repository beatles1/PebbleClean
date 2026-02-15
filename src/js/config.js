module.exports = [
  {
    "type": "heading",
    "defaultValue": "WatchfaceConfiguration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colours"
      },
      {
        "type": "color",
        "messageKey": "HealthBarColour",
        "defaultValue": "0xAA00FF",
        "sunlight": true,
        "label": "Health Bar"
      },
      {
        "type": "color",
        "messageKey": "BatteryBarColour",
        "defaultValue": "0x55AAAA",
        "sunlight": true,
        "label": "Battery Bar"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Health Bar"
      },
      {
        "type": "select",
        "messageKey": "HealthMetric",
        "label": "Metric",
        "defaultValue": 1,
        "options": [
          { 
            "label": "Steps",
            "value": 1
          },
          { 
            "label": "Active Time (Minutes)",
            "value": 2
          },
          { 
            "label": "Walked Distance (Meters)",
            "value": 3
          },
          { 
            "label": "Sleep length (Minutes)",
            "value": 4
          },
          { 
            "label": "Active calories (Kcal)",
            "value": 5
          },
        ]
      },
      {
        "type": "input",
        "messageKey": "Target",
        "defaultValue": "10000",
        "label": "Target",
        "attributes": {
          "type": "number",
          "min": 1,
          "max": 100000
        }
      },
      {
        "type": "text",
        "defaultValue": "The health bar will be will width when the value reaches the target set above."
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
