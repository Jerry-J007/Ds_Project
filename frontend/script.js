// Example: Sending a vote to the C++ backend
async function castVote(event) {
    event.preventDefault(); // Prevent page reload
    
    const rollNo = document.getElementById('rollInput').value;
    const candidateID = document.getElementById('candidateSelect').value;

    // Send data as URL-encoded form data
    const formData = new URLSearchParams();
    formData.append('roll', rollNo);
    formData.append('candidate', candidateID);

    try {
        const response = await fetch('/api/vote', {
            method: 'POST',
            body: formData
        });
        
        const data = await response.json(); // Parse the JSON from C++
        
        if (data.status === 'success') {
            document.getElementById('statusMessage').innerText = "✅ " + data.message;
            document.getElementById('statusMessage').style.color = "green";
        } else {
            document.getElementById('statusMessage').innerText = "❌ " + data.message;
            document.getElementById('statusMessage').style.color = "red";
        }
    } catch (error) {
        console.error("Server error:", error);
    }
}

// Bind to your HTML form
document.getElementById('voteForm').addEventListener('submit', castVote);