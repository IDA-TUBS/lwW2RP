import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import datetime

# Function to parse log lines and extract relevant information
def parse_log_line(line):
    parts = line.split(',')
    timestamp = datetime.datetime.strptime(parts[0], '%Y-%m-%d %H:%M:%S.%f')
    event_type = parts[1].strip()
    events = ["SAMPLE_START", "SAMPLE_END"]
    sequence_number = int(parts[2]) if event_type in events else None
    # print(sequence_number)
    return timestamp, event_type, sequence_number

# Read log data into a Pandas DataFrame
log_data = []
with open('writer_test_video_20241014_134040.log', 'r') as f:
    for line in f:
        log_data.append(parse_log_line(line))
df = pd.DataFrame(log_data, columns=['Timestamp', 'Event Type', 'Sequence Number'])

df = df[(df['Event Type'] != 'NACKFRAG')]
df = df[(df['Event Type'] != 'SN Arrival')]

df['Timestamp'] = pd.to_datetime(df['Timestamp'])

print(df)


start_df = df[df['Event Type'] == 'SAMPLE_START']
end_df = df[df['Event Type'] == 'SAMPLE_END']
# Merge the two dataframes on 'Sequence Number' to get matching START and END times
merged_df = pd.merge(start_df, end_df, on='Sequence Number', suffixes=('_start', '_end'))
# Calculate the difference between the timestamps in microseconds
merged_df['time_diff_us'] = (merged_df['Timestamp_end'] - merged_df['Timestamp_start']).dt.total_seconds() * 1e6

# Select relevant columns to display the result
result = merged_df[['Sequence Number', 'Timestamp_start', 'Timestamp_end', 'time_diff_us']]
# Display the result
print(result)




# Calculate the difference between subsequent SAMPLE_START entries
start_df = start_df.sort_values(by='Timestamp')  # Sort by Timestamp to ensure correct order
start_df['start_diff_us'] = start_df['Timestamp'].diff().dt.total_seconds() * 1e6

# Drop the first row as it will have a NaN value for the time difference
start_df = start_df.dropna(subset=['start_diff_us'])








# Create the plot with two y-axes
fig, ax1 = plt.subplots(figsize=(10, 6))

# Plot for Start-End time differences (left y-axis)
color = 'tab:blue'
ax1.set_xlabel('Sequence Number')
ax1.set_ylabel('Start-End Time Difference (us)', color=color)
ax1.plot(result['Sequence Number'], result['time_diff_us'], marker='o', linestyle='-', color=color, label='Start-End Time Difference (us)')
ax1.tick_params(axis='y', labelcolor=color)
ax1.grid(True)

# Create the second y-axis for Start-Start time differences
ax2 = ax1.twinx()
color = 'tab:red'
ax2.set_ylabel('Start-Start Time Difference (us)', color=color)
ax2.plot(start_df['Sequence Number'], start_df['start_diff_us'], marker='x', linestyle='--', color=color, label='Start-Start Time Difference (us)')
ax2.tick_params(axis='y', labelcolor=color)

# Add titles and show the plot
plt.title('Time Differences: Start-End and Start-Start (per Sequence Number)')
fig.tight_layout()  # Adjust layout to make room for both y-axes








# Create a violin plot
plt.figure(figsize=(8, 6))
sns.violinplot(data=merged_df, x='Event Type_start', y='time_diff_us', inner='box', color='skyblue')

# Add titles and labels
plt.title('Distribution of Start-End Time Differences')
plt.xlabel('Event Type')
plt.ylabel('Time Difference (Microseconds)')





plt.show()
