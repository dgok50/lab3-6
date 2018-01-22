/*
 * testf.c
 *
 *  Created on: 12.02.2013
 *      Author: dgok50
 */

#include <stdio.h>
#include <func.c>
#include <def.h>

void main(int argc, char *argv[]) {
    int f, d = 0;
    if (argc > 1) {
        for(d=0; d<=argc; d++)
        {
            if (!strcmp(argv[d], "--rebuild_sh") && !strcmp(argv[d], "-R")) {
                char confnd[256];
                sprintf(confnd, "%s/%s", CONFIG_DIR, CONFIG_FILE);
                f=cre_corn_sh("term_xml_gen", confnd);
                exit(f);
            }
            if (!strcmp(argv[d], "--help")) {
                printf("Cre Conf ver: %s\nКлючи запуска:\n --rebuild_sh -R пересоздать файлы конфигурации\n --help вывод данной справки\n", ver);
                exit(0);
            }
        }
    }
    else {
        f = cre_conf(CONFIG_DIR, CONFIG_FILE);
        if (f != 0)
            perror("Не возможно создать конфиг\n");
        exit(f);
    }
}
