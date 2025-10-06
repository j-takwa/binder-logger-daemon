#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Trim leading/trailing whitespace/newline
char* trim(char* str) {
    while (*str == ' ') str++;
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n' || *end == '\r')) *end-- = '\0';
    return str;
}

// Extract nth occurrence of key=value from line (n=1 for first)
char* extract_field_n(const char* line, const char* key, int n) {
    const char* p = line;
    int count = 0;
    while (p) {
        const char* found = strstr(p, key);
        if (!found) break;
        count++;
        if (count == n) {
            found += strlen(key);
            const char* end = strchr(found, ',');
            if (!end) end = strchr(found, '\0');
            size_t len = end - found;
            char* value = (char*)malloc(len + 1);
            strncpy(value, found, len);
            value[len] = '\0';
            return trim(value);
        } else {
            p = found + strlen(key);
        }
    }
    return NULL;
}

// Parse success field
const char* parse_success(const char* line) {
    char* success_val = extract_field_n(line, "success=", 1);
    if (!success_val) return "fail";
    const char* result = (strcmp(success_val, "true") == 0) ? "success" : "fail";
    printf("DEBUG: parsed success='%s'\n", success_val);
    fflush(stdout);
    free(success_val);
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <logfile>\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (!fp) { perror("fopen"); return 1; }

    char line[8192];
    int call_counter = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (!strstr(line, "changes_by_abdu_2")) continue;

        call_counter++;

        char* ts             = extract_field_n(line, "timestamp=", 1);
        char* method         = extract_field_n(line, "method=", 1);
        char* target_service = extract_field_n(line, "target_service=", 1);
        char* caller_uid     = extract_field_n(line, "caller=uid=", 1);
        char* caller_pid     = extract_field_n(line, "pid=", 1);
        char* binder_instance= extract_field_n(line, "binder=", 1); // first binder
        char* binder_object  = extract_field_n(line, "binder=", 2); // second binder (real BinderProxy)
        char* interface      = extract_field_n(line, "interface=", 1);
        char* duration       = extract_field_n(line, "durationUs=", 1);
        char* fetched        = extract_field_n(line, "fetched_from_servicemanager=", 1);
        const char* status   = parse_success(line);

        printf("==================== BINDER CALL %d ====================\n", call_counter);
        printf("Timestamp : %s\n\n", ts ? ts : "unknown");

        // CLIENT block
        printf("+---------------------------------------+\n");
        printf("| CLIENT (caller)                       |\n");
        printf("| uid     : %s\n", caller_uid ? caller_uid : "unknown");
        printf("| pid     : %s\n", caller_pid ? caller_pid : "unknown");
        printf("| instance: %s\n", binder_instance ? binder_instance : "null");
        printf("| status  : %s\n", strcmp(status,"success")==0 ? "success" : "fail");
        printf("+---------------------------------------+\n");
        printf("               |\n               v\n");

        // BINDER INFO block
        printf("+---------------------------------------+\n");
        printf("| BINDER INFO                           |\n");
        printf("| binder id : %s\n", binder_object ? binder_object : "null");
        printf("| method    : %s\n", method ? method : "unknown");
        if(duration) printf("| duration  : %s µs\n", duration);
        printf("+---------------------------------------+\n");
        printf("               |\n               v\n");

        // SERVICE block
        printf("+---------------------------------------+\n");
        printf("| SERVICE (target)                      |\n");
        printf("| name     : %s\n", target_service ? target_service : "unknown");
        printf("| interface: %s\n", interface ? interface : "unknown");
        if(fetched) printf("| source   : %s\n", strcmp(fetched,"true")==0 ? "ServiceManager" : "Unknown");
        else printf("| source   : Unknown\n");
        printf("| cached   : false\n");
        printf("+---------------------------------------+\n");

        printf("Result: %s\n", strcmp(status,"success")==0 ? "✅ Request served successfully" : "❌ Request failed");
        printf("------------------------------------------------------------\n\n");

        // Free allocated strings
        free(ts); free(method); free(target_service);
        free(caller_uid); free(caller_pid);
        free(binder_instance); free(binder_object);
        free(interface); free(duration); free(fetched);
    }

    fclose(fp);
    return 0;
}

