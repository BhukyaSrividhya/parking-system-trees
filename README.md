# parking-system-trees
This project is a C-based command-line application for managing a smart car parking lot with 50 spaces, leveraging B+ Trees for efficient storage, retrieval, and sorting of vehicle and parking space data.
Efficient Data Management:
Uses B+ Trees to store and manage vehicles and parking spaces, enabling fast search, insertion, and sorted traversals.

Parking Operations:

Register new vehicles and owners.
Allocate parking spaces based on membership (Gold, Premium, None) with nearest-space policy.
Process vehicle exits, calculate parking fees, and update membership status automatically.
Membership & Payment Policies:

Membership upgrades based on total parking hours (Gold, Premium, None).
Automated fee calculation with discounts for members.
Reporting & Analytics:

Display all vehicles and parking spaces.
Sort and display vehicles by parking count or total amount paid.
Sort and display parking spaces by occupancy or revenue.
Persistent Storage:
Loads and saves all vehicle data to vehicles_text.txt for data persistence across sessions.

User-Friendly CLI:
Menu-driven interface for all operations and reports.
