#include <gtk/gtk.h>

/* enum for conf_tree columns */
enum {
	CONF_COL_NAME = 0,
	CONF_COL_ABBR,
	CONF_COL_VALUE,
	CONF_NUM_COLS
};

GtkTreeStore *
create_and_fill_store() {
	GtkTreeStore *store = gtk_tree_store_new(
		CONF_NUM_COLS,
		G_TYPE_STRING, /* command name */
		G_TYPE_STRING, /* abbreviation */
		G_TYPE_STRING  /* current value */
	);

	GtkTreeIter category, command;

	gtk_tree_store_append(store, &category, NULL);
	gtk_tree_store_set(store, &category, 0, "Networking", -1);

	gtk_tree_store_append(store, &command, &category);
	gtk_tree_store_set(store, &command,
		CONF_COL_NAME, "PAN ID",
		CONF_COL_ABBR, "ID",
		CONF_COL_VALUE, "0",
		-1);

	return store;
}

void
set_main_view_columns(GtkTreeView *view) {
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			"Name",
			renderer,
			"text", CONF_COL_NAME,
			NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(view, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			"Abbr",
			renderer,
			"text", CONF_COL_ABBR,
			NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(view, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(
			"Value",
			renderer,
			"text", CONF_COL_VALUE,
			NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(view, column);
}

void
setup_tree_view(GtkTreeView *view) {
	GtkTreeStore *store = create_and_fill_store();

	gtk_tree_view_set_model(view, GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	set_main_view_columns(view);
}

int
main(int argc, char *argv[]) {
	GtkBuilder *builder;
	GtkWidget *window, *tree;
	GError *error = NULL;

	gtk_init(&argc, &argv);
	builder = gtk_builder_new();

	if(!gtk_builder_add_from_file(builder, "xbee-comm.glade", &error)) {
		g_warning("%s", error->message);
		g_free(error);
		return 1;
	}

	gtk_builder_connect_signals(builder, NULL);

	window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
	tree   = GTK_WIDGET(gtk_builder_get_object(builder, "conf_treeview"));

	setup_tree_view(GTK_TREE_VIEW(tree));

	g_object_unref(G_OBJECT(builder));

	/* all done setting up the main window, so make it visible! */
	gtk_widget_show(window);

	/* loop. */
	gtk_main();

	return 0;
}
