#include <gtk/gtk.h>
#include <stdlib.h>

typedef struct {
	double first_operand;
	double second_operand;
	char operation;
	gboolean is_second_operand;
	GtkEntry *display;
} CalculatorState;

void on_button_clicked(GtkButton *button, gpointer user_data) {
	CalculatorState *state = (CalculatorState *)user_data;
	const char *button_label = gtk_button_get_label(button);
	GtkEntry *display = state->display;
	const char *current_text = gtk_entry_get_text(display);

	// Clear button logic
	if (g_strcmp0(button_label, "C") == 0) {
		gtk_entry_set_text(display, "");
		state->first_operand = 0;
		state->second_operand = 0;
		state->operation = '\0';
		state->is_second_operand = FALSE;
		return;
	}

	// Equal button logic
	if (g_strcmp0(button_label, "=") == 0) {
		if (state->operation != '\0' && state->is_second_operand) {
			state->second_operand = atof(current_text);
			double result = 0;

			switch (state->operation) {
				case '+': result = state->first_operand + state->second_operand; break;
				case '-': result = state->first_operand - state->second_operand; break;
				case '*': result = state->first_operand * state->second_operand; break;
				case '/': 
						  if (state->second_operand != 0)
							  result = state->first_operand / state->second_operand; 
						  else
							  gtk_entry_set_text(display, "Error: Division by Zero");
						  return;
				default: break;
			}

			char result_str[50];
			snprintf(result_str, sizeof(result_str), "%g", result);
			gtk_entry_set_text(display, result_str);

			// Reset state
			state->first_operand = result;
			state->operation = '\0';
			state->is_second_operand = FALSE;
		}
		return;
	}

	// Operation button logic
	if (g_strcmp0(button_label, "+") == 0 || g_strcmp0(button_label, "-") == 0 ||
			g_strcmp0(button_label, "*") == 0 || g_strcmp0(button_label, "/") == 0) {
		state->first_operand = atof(current_text);
		state->operation = button_label[0];
		state->is_second_operand = TRUE;
		gtk_entry_set_text(display, "");
		return;
	}

	// Number button logic
	char *new_text = g_strdup_printf("%s%s", current_text, button_label);
	gtk_entry_set_text(display, new_text);
	g_free(new_text);
}

int main(int argc, char *argv[]) {
	GtkBuilder *builder;
	GtkWidget *window;
	GError *error = NULL;

	CalculatorState state = {0, 0, '\0', FALSE};

	gtk_init(&argc, &argv);

	// Load the UI description
	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, "calculator_ui.xml", &error)) {
		g_printerr("Error loading file: %s\n", error->message);
		g_clear_error(&error);
		return 1;
	}

	// Get main window
	window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
	if (!window) {
		g_printerr("Unable to find main_window\n");
		return 1;
	}

	// Get the display entry
	state.display = GTK_ENTRY(gtk_builder_get_object(builder, "edit_display"));
	if (!state.display) {
		g_printerr("Unable to find edit_display\n");
		return 1;
	}

	// Connect button signals
	const char *button_ids[] = {
		"button_0", "button_1", "button_2", "button_3", "button_4",
		"button_5", "button_6", "button_7", "button_8", "button_9",
		"button_clear", "button_calc", "button_sum", "button_sub",
		"button_mul", "button_div", NULL
	};

	for (int i = 0; button_ids[i] != NULL; i++) {
		GtkWidget *button = GTK_WIDGET(gtk_builder_get_object(builder, button_ids[i]));
		if (!button) {
			g_printerr("Unable to find %s\n", button_ids[i]);
			continue;
		}
		g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), &state);
	}

	// Show all widgets
	gtk_widget_show_all(window);

	// Connect destroy signal
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	// Start GTK main loop
	gtk_main();

	return 0;
}