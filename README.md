# uxn

## labels

|type  |                |         |size     |
|------|----------------|---------|---------|
|.label|literal address |zero page|one byte |
|;label|literal address |         |one short|
|,label|relative address|         |one byte |
|:label|raw address     |         |one short|

    @label
    &sub-label
    label/sub-label

