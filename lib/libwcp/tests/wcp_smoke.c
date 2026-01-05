#include <glib.h>

#include "wcp_protocol.h"

static void test_parse_add_items(void)
{
    const char *json =
        "{\"type\":\"command\",\"command\":\"add_items\","
        "\"items\":[\"top.sig\",\"top.bus\"],\"recursive\":true}";
    GError *error = NULL;
    WcpCommand *cmd = wcp_parse_command(json, &error);

    g_assert_no_error(error);
    g_assert_nonnull(cmd);
    g_assert_cmpint(cmd->type, ==, WCP_CMD_ADD_ITEMS);
    g_assert_nonnull(cmd->data.add_items.items);
    g_assert_cmpuint(cmd->data.add_items.items->len, ==, 2);
    g_assert_true(cmd->data.add_items.recursive);
    g_assert_cmpstr(g_ptr_array_index(cmd->data.add_items.items, 0), ==, "top.sig");
    g_assert_cmpstr(g_ptr_array_index(cmd->data.add_items.items, 1), ==, "top.bus");

    wcp_command_free(cmd);
}

static void test_parse_invalid_json(void)
{
    const char *json = "{\"type\":\"command\"}";
    GError *error = NULL;
    WcpCommand *cmd = wcp_parse_command(json, &error);

    g_assert_null(cmd);
    g_assert_nonnull(error);
    g_clear_error(&error);
}

static void test_greeting(void)
{
    char *greeting = wcp_create_greeting();
    g_assert_nonnull(greeting);
    g_assert_nonnull(g_strstr_len(greeting, -1, "\"type\":\"greeting\""));
    g_assert_nonnull(g_strstr_len(greeting, -1, "\"get_item_list\""));
    g_assert_nonnull(g_strstr_len(greeting, -1, "\"add_items\""));
    g_free(greeting);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/libwcp/parse_add_items", test_parse_add_items);
    g_test_add_func("/libwcp/parse_invalid_json", test_parse_invalid_json);
    g_test_add_func("/libwcp/greeting", test_greeting);

    return g_test_run();
}
