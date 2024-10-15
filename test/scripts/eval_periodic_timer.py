import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import re
import datetime
import sys


# Specify the log file path
log_file_path = sys.argv[1]

# Open and read the file
with open(log_file_path, 'r') as file:
    log_data = file.read()

# Regular expression pattern to extract timestamp, event type, and sequence number
pattern = r'\[(.*?)\] .+{ string_field = "(.*?)", integer_field = (\d+) }'

# Parse the log file
matches = re.findall(pattern, log_data)

# Create DataFrame
df = pd.DataFrame(matches, columns=['Timestamp', 'Event Type', 'Sequence Number'])

df['Event Type'] = df['Event Type'].apply(lambda x: re.sub(r'[^a-zA-Z_]', '', x))

# Convert Sequence Number to integer type
df['Sequence Number'] = df['Sequence Number'].astype(int)
df['Timestamp'] = pd.to_datetime(df['Timestamp'])
print(df)


start_df = df[df['Event Type'] == 'SAMPLE_START']


# Calculate the difference between subsequent SAMPLE_START entries
start_df = start_df.sort_values(by='Timestamp')  # Sort by Timestamp to ensure correct order
start_df['start_diff_us'] = start_df['Timestamp'].diff().dt.total_seconds() * 1e6

# Drop the first row as it will have a NaN value for the time difference
start_df = start_df.dropna(subset=['start_diff_us'])
print(start_df)

# Create the plot with two y-axes
fig, ax1 = plt.subplots(figsize=(10, 6))

# Plot for Start-End time differences (left y-axis)
color = 'tab:blue'
ax1.set_xlabel('Sequence Number')
ax1.set_ylabel('Start-Start Time Difference (us)', color=color)
ax1.plot(start_df['Sequence Number'], start_df['start_diff_us'], marker='x', linestyle='--', color=color, label='Start-Start Time Difference (us)')
ax1.tick_params(axis='y', labelcolor=color)
ax1.grid(True)

# Add titles and show the plot
plt.title('Time Differences: Start-Start (per Sequence Number)')
fig.tight_layout()  # Adjust layout to make room for both y-axes

# Create a violin plot
plt.figure(figsize=(8, 6))
sns.violinplot(data=start_df, x='start_diff_us', inner='box', color='skyblue')

# Add titles and labels
plt.title('Distribution of Start-End Time Differences')
plt.xlabel('Event Type')
plt.ylabel('Time Difference (Microseconds)')

plt.show()
