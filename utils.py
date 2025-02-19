import matplotlib.pyplot as plt
from IPython import display

def setup_plots():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 5))
    loss_line, = ax1.plot([], [], 'b-')
    acc_line, = ax2.plot([], [], 'r-')

    ax1.set_xlabel('Iteration')
    ax1.set_ylabel('Loss')
    ax1.set_title('Training Loss')

    ax2.set_xlabel('Iteration')
    ax2.set_ylabel('Accuracy')
    ax2.set_title('Training Accuracy')

    return fig, ax1, ax2, loss_line, acc_line

def update_plots(losses, accuracies, iterations, fig, ax1, ax2, loss_line, acc_line):
    # Update both line data
    loss_line.set_xdata(iterations)
    loss_line.set_ydata(losses)
    acc_line.set_xdata(iterations)
    acc_line.set_ydata(accuracies)

    # Adjust plot limits for both plots
    ax1.relim()
    ax1.autoscale_view()
    ax2.relim()
    ax2.autoscale_view()
    
    # Redraw the plot
    display.clear_output(wait=True)
    display.display(fig)