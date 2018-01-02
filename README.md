## mbedOS Lablet I2C

### Sample per la comunicazione via I2C con sensore di temperatura "intelligente" Microchip MCP9801

Il pin di alert (che è un'uscita open-drain) va pull-uppato verso VDD; per default la polarità è "active-low" (ma può essere invertita da registro di configurazione). Per default la logica di alerting è di tipo "comparatore", con alert attivo se la temperatura è al di fuori dell'intervallo TH-TL (nomenclatura del datasheet); l'altra modalità è "interrupt", in cui l'alert viene attivato al superamento di una soglia (TA sale oltre TL o scende sotto TH) e resta attivo fino alla successiva lettura di un registro.  