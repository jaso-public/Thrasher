import numpy as np
import PyQt5
import PySide2
import matplotlib
import matplotlib.pyplot as plt

matplotlib.use('TkAgg')


def read_file_to_numpy_array(file_path):
    # Initialize an empty list to store the integers
    data = []

    # Open the file and read line by line
    with open(file_path, 'r') as file:
        for line in file:
            # Convert each line to an integer and append to the list
            data.append(int(line.strip()))

    # Convert the list to a NumPy array
    return np.array(data)

# Example usage

def bucketize(data) :
    values = []
    index = 0;
    for s in range(1,5) :
        for b in range(1,10) :
            end = (10 ** s) * b 
            count = 0
            while index < end :
                count += data[index]
                index += 1

            list.append(values, [end, count])
            print(f'{end} {count}')
    return values


b = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000]


file_path = '/users/jaso/Thrasher/build/foo.out'  # Replace with your file path

data = read_file_to_numpy_array(file_path)
data = bucketize(data)

x_values = [str(x[0]) for x in data]
frequencies = [x[1] for x in data]

plt.figure(figsize=(12, 6))

# Creating the bar plot
plt.bar(x_values, frequencies, width=0.8, align='center')

# Setting the labels and title
plt.xlabel('Value')
plt.ylabel('Frequency')
plt.title('Histogram of Frequencies')

# Rotate x-labels for better readability
plt.xticks(rotation=45)
plt.yscale('log')

# Show the plot
plt.show()


