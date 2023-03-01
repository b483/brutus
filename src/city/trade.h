#ifndef CITY_TRADE_H
#define CITY_TRADE_H

void city_trade_update(void);

int city_trade_next_caravan_import_resource(void);
int city_trade_next_caravan_backup_import_resource(void);

int city_trade_next_docker_import_resource(void);
int city_trade_next_docker_export_resource(void);

#endif // CITY_TRADE_H
