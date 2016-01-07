function Set(array_of_elements, params) {
	this.set_id = 0;
	this.elements_hash = {};
	this.params = params || {};
}

Set.prototype.get_id = function (element) {
	if (element.__set_id) {
		return element.__set_id;
	} else if (this.params.id_func) {
		return element.__set_id = this.params.id_func(element);
	} else {
		return element.__set_id = ++this.set_id;
	}
}

Set.prototype.add = function (element) {
	var element_id = this.get_id(element);
	if (this.count(element) == 0) {
		this.elements_hash[element_id] = element;
	}
}

Set.prototype.count = function (element) {
	var element_id = this.get_id(element);
	if (this.elements_hash[element_id]) {
		return 1;
	} else {
		return 0;
	}
}

Set.prototype.remove = function (element) {
	var element_id = this.get_id(element);
	if (this.count(element)) {
		delete(this.elements_hash[element_id]);
	}
}

Set.prototype.each = function (apply_to_element_func) {
	for (var element_id in this.elements_hash) {
		var element = this.elements_hash[element_id];
		apply_to_element_func(element);
	}
}

Set.prototype.get = function(element) {
	if (this.count(element)) {
		var element_id = this.get_id(element);
		return this.elements_hash[element_id];
	} else {
		throw new Error("There is no such element in set");
	}
}

Set.prototype.size = function () {
	return Object.keys(this.elements_hash).length;
}
