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