function renameFile(path, curr_name) {
    const newName = prompt("New name:", curr_name);

    if (!newName)
        return;

    const form = document.createElement("form");
    form.method = "POST";
    form.action = path + "/rename";

    const input = document.createElement("input");
    input.type = "hidden";
    input.name = "new_name";
    input.value = newName;

    form.appendChild(input);
    document.body.appendChild(form);
    form.submit();
}

function toggleMenu(button) {
    const menu = button.nextElementSibling;
    
     // close every other menu
    document.querySelectorAll(".menu").forEach(m => {
        if (m !== menu)
            m.style.display = "none";
    });

    menu.style.display =
        menu.style.display === "block"
            ? "none"
            : "block";
}

document.addEventListener("click", (e) => {
    if (!e.target.closest(".menu-container")) {
        document.querySelectorAll(".menu").forEach(menu => {
            menu.style.display = "none";
        });
    }
} );