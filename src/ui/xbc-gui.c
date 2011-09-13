#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <gtk/gtk.h>

/* enum for conf_tree columns */
enum {
	CONF_COL_NAME = 0,
	CONF_COL_ABBR,
	CONF_COL_VALUE_DEF,
	CONF_COL_VALUE_DESC,
	CONF_COL_DESC,
	CONF_NUM_COLS
};

struct xbc_gui_ctx {
	GtkWidget *window;
	GtkWidget *tree;
	GtkWidget *text;
};

void
store_add_category(GtkTreeStore *store, GtkTreeIter *iter, const char *category) {
	gtk_tree_store_append(store, iter, NULL);
	gtk_tree_store_set(store, iter, 0, category, -1);
}

void
store_add_command(GtkTreeStore *store, GtkTreeIter *iter, char *params[CONF_NUM_COLS]) {
	GtkTreeIter child;
	gtk_tree_store_append(store, &child, iter);
	gtk_tree_store_set(store, &child,
		CONF_COL_NAME, params[CONF_COL_NAME],
		CONF_COL_ABBR, params[CONF_COL_ABBR],
		CONF_COL_VALUE_DEF, params[CONF_COL_VALUE_DEF],
		CONF_COL_VALUE_DESC, params[CONF_COL_VALUE_DESC],
		CONF_COL_DESC, params[CONF_COL_DESC],
		-1);
}

enum read_conf_enum {
	XBC_READ_CONF_START,
	XBC_READ_CONF_CATEGORY,
	XBC_READ_CONF_COMMAND
};

int
read_conf_file(GtkTreeStore *store, const char *filename) {
	char *buf, *line, *ptr, *type, *title, *desc, *tok;
	char *params[CONF_NUM_COLS];
	int fd;
	size_t filesize, off;
	ssize_t ret;
	enum read_conf_enum state;
	struct stat stbuf;
	GtkTreeIter iter;

	if( (fd = open(filename, O_RDONLY)) < 0) {
		g_warning("failed to open %s", filename);
		return -1;
	}

	if(fstat(fd, &stbuf) < 0) {
		g_warning("failed to stat %s", filename);
		close(fd);
		return -1;
	}

	/* for now, ignore files greater than 64K */
	if(stbuf.st_size <= 0 || stbuf.st_size > 64 * 1024) {
		g_warning("unexpected file size");
		close(fd);
		return -1;
	}

	filesize = (size_t)stbuf.st_size;
	if(!(buf = (char *)malloc(filesize + 1))) {
		g_warning("malloc failed");
		close(fd);
		return -1;
	}
	buf[filesize] = '\0';

	off = 0;
	do {
		ret = read(fd, buf + off, filesize - off);

		if(ret < 0) {
			g_warning("error reading configuration file");
			free(buf);
			close(fd);
			return -1;
		}

		off += (size_t)ret;
	} while(ret);

	state = XBC_READ_CONF_START;
	for(line = ptr = buf; *ptr; ptr++) {
		/* EOL? */
		if(*ptr == '\r' || *ptr == '\n') {
			if((*(ptr + 1) == '\r' || *(ptr + 1) == '\n') && *ptr != (*ptr + 1)) {
				*(ptr++) = '\0';
			}
			*ptr = '\0';

			/* skip blank lines */
			if(!*line) {
				if(state == XBC_READ_CONF_COMMAND)
					state = XBC_READ_CONF_CATEGORY;
				line = ptr + 1;
				continue;
			}

			/* skip lines that don't start with a '[' */
			if(*line != '[') {
				g_warning("malformed line: %s", line);
				line = ptr + 1;
				continue;
			}
			line++;

			/* parse the line */
			switch(state) {
			case XBC_READ_CONF_START:
				/* read header line */
				state = XBC_READ_CONF_CATEGORY;
				break;
			case XBC_READ_CONF_CATEGORY:
				/* category type */
				if(!(tok = strchr(line, ']')) || *(tok + 1) != '[') {
					g_warning("failed to find category type");
					break;
				}
				*(tok++) = '\0';
				type = line;
				line = ++tok;
				/* category title */
				if(!(tok = strchr(line, ']')) || *(tok + 1) != '[') {
					g_warning("failed to find category title");
					break;
				}
				*(tok++) = '\0';
				title = line;
				line = ++tok;
				/* category description */
				if(!(tok = strchr(line, ']'))) {
					g_warning("failed to find category description");
					break;
				}
				*(tok++) = '\0';
				desc = line;
				/* add the category and switch to reading commands */
				store_add_category(store, &iter, title);
				state = XBC_READ_CONF_COMMAND;
				break;
			case XBC_READ_CONF_COMMAND:
				/* command abbreviation */
				if(!(tok = strchr(line, ']')) || *(tok + 1) != '[') {
					g_warning("failed to find command abbreviation");
					break;
				}
				*(tok++) = '\0';
				params[CONF_COL_ABBR] = line;
				line = ++tok;
				/* command default */
				if(!(tok = strchr(line, ']')) || *(tok + 1) != '[') {
					g_warning("failed to find command default");
					break;
				}
				*(tok++) = '\0';
				params[CONF_COL_VALUE_DEF] = line;
				line = ++tok;
				/* command name */
				if(!(tok = strchr(line, ']')) || *(tok + 1) != '[') {
					g_warning("failed to find command name");
					break;
				}
				*(tok++) = '\0';
				params[CONF_COL_NAME] = line;
				line = ++tok;
				/* command value description */
				if(!(tok = strchr(line, ']')) || *(tok + 1) != '[') {
					g_warning("failed to find command value description");
					break;
				}
				*(tok++) = '\0';
				params[CONF_COL_VALUE_DESC] = line;
				line = ++tok;
				/* command description */
				if(!(tok = strchr(line, ']'))) {
					g_warning("failed to find command description");
					break;
				}
				*(tok++) = '\0';
				params[CONF_COL_DESC] = line;
				line = ++tok;
				/* add the command */
				store_add_command(store, &iter, params);
				break;
			}

			line = ptr + 1;
		}
	}

	free(buf);
	close(fd);

	return 0;
}

gboolean
row_select_cb(GtkTreeSelection *sel, GtkTreeModel *model, GtkTreePath *path,
		gboolean cur_selected, gpointer userdata) {
	gchar *desc;
	GtkTextBuffer *buffer;
	GtkTextView *text;
	GtkTreeIter iter;
	struct xbc_gui_ctx *gctx;

	if(gtk_tree_path_get_depth(path) == 1) {
		return FALSE;
	}

	if(gtk_tree_model_get_iter(model, &iter, path)) {
		gtk_tree_model_get(model, &iter, CONF_COL_DESC, &desc, -1);

		gctx = (struct xbc_gui_ctx *)userdata;
		text = GTK_TEXT_VIEW(gctx->text);
		buffer = gtk_text_view_get_buffer(text);

		if(!cur_selected) { /* this row is being selected */
			gtk_text_buffer_set_text(buffer, desc, -1);
		}
		else {
			gtk_text_buffer_set_text(buffer, "", 0);
		}

		g_free(desc);
	}

	return TRUE;
}

GtkTreeStore *
create_and_fill_store() {
	GtkTreeStore *store = gtk_tree_store_new(
		CONF_NUM_COLS,
		G_TYPE_STRING, /* command name */
		G_TYPE_STRING, /* abbreviation */
		G_TYPE_STRING, /* default value */
		G_TYPE_STRING, /* value description */
		G_TYPE_STRING  /* command description */
	);

	read_conf_file(store, "XB24-ZB_2070.mxi");

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
			"text", CONF_COL_VALUE_DEF,
			NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(view, column);
}

void
setup_tree_view(GtkTreeView *view, struct xbc_gui_ctx *gctx) {
	GtkTreeSelection *selection;
	GtkTreeStore *store = create_and_fill_store();

	gtk_tree_view_set_model(view, GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	/* create column headers */
	set_main_view_columns(view);

	/* at most one row may be selected at a time */
	selection = gtk_tree_view_get_selection(view);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	/* hook up the row select callback */
	gtk_tree_selection_set_select_function(selection, row_select_cb, gctx, NULL);
}

gboolean
xbc_gui_init(struct xbc_gui_ctx *gctx) {
	GtkBuilder *builder;
	GError *error = NULL;

	builder = gtk_builder_new();

	if(!gtk_builder_add_from_file(builder, "xbee-comm.glade", &error)) {
		g_warning("%s", error->message);
		g_free(error);
		return FALSE;
	}

	gctx->window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
	gctx->tree   = GTK_WIDGET(gtk_builder_get_object(builder, "conf_treeview"));
	gctx->text   = GTK_WIDGET(gtk_builder_get_object(builder, "conf_help"));

	/* pass gctx to signal handlers */
	gtk_builder_connect_signals(builder, gctx);

	setup_tree_view(GTK_TREE_VIEW(gctx->tree), gctx);

	g_object_unref(G_OBJECT(builder));

	return TRUE;
}

int
main(int argc, char *argv[]) {
	struct xbc_gui_ctx *gui_ctx;

	gtk_init(&argc, &argv);

	/* setup gui */
	gui_ctx = g_slice_new(struct xbc_gui_ctx);
	if(!xbc_gui_init(gui_ctx)) {
		return 1;
	}

	/* all done setting up the main window, so make it visible! */
	gtk_widget_show(gui_ctx->window);

	/* loop. */
	gtk_main();

	g_slice_free(struct xbc_gui_ctx, gui_ctx);

	return 0;
}
